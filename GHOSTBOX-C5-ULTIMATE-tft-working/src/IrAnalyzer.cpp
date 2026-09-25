#include "IrAnalyzer.h"

#include <Arduino.h>

#include "PepeDraw.h"
#include "Pins.h"

static constexpr uint16_t ANALYZER_RAW_MAX        = 96;
static constexpr uint8_t  ANALYZER_RAW_PREVIEW    = 10;
static constexpr uint32_t ANALYZER_MIN_PULSE_US   = 180;
static constexpr uint32_t ANALYZER_START_MIN_US   = 1800;
static constexpr uint32_t ANALYZER_START_MAX_US   = 20000;
static constexpr uint32_t ANALYZER_FRAME_GAP_US   = 25000;
static constexpr uint32_t ANALYZER_FRAME_MAX_US   = 220000;
static constexpr uint16_t ANALYZER_DRAW_MS        = 250;
static constexpr uint16_t ANALYZER_ACTIVE_MS      = 650;
static constexpr uint16_t ANALYZER_REPEAT_MS      = 650;

enum IrAnalyzerSignalState {
    IR_SIGNAL_IDLE = 0,
    IR_SIGNAL_FRAME,
    IR_SIGNAL_REPEAT,
    IR_SIGNAL_NOISE
};

struct IrAnalyzerState {
    int level = HIGH;
    uint32_t lastChangeUs = 0;
    uint32_t lastEdgeUs = 0;
    uint32_t frameStartUs = 0;
    bool frameActive = false;
    bool frameOverflow = false;

    uint16_t raw[ANALYZER_RAW_MAX];
    uint16_t rawCount = 0;

    uint16_t lastRawPreview[ANALYZER_RAW_PREVIEW];
    uint8_t lastRawPreviewCount = 0;
    uint16_t lastRawCount = 0;
    uint32_t lastFrameTotalUs = 0;
    uint16_t lastHeaderUs = 0;
    uint16_t lastFrameGapMs = 0;
    bool lastOverflow = false;
    unsigned long lastFrameMs = 0;
    unsigned long lastNoiseMs = 0;

    unsigned long totalEdges = 0;
    unsigned long totalLowPulses = 0;
    unsigned long totalFrames = 0;
    unsigned long totalNoise = 0;

    unsigned long windowStartMs = 0;
    unsigned long windowEdges = 0;
    unsigned long windowLowPulses = 0;
    unsigned long windowFrames = 0;
    unsigned long windowNoise = 0;
    uint16_t edgesPerSec = 0;
    uint16_t lowPulsesPerSec = 0;
    uint16_t framesPerSec = 0;
    uint16_t noisePerSec = 0;
    unsigned long lastActivityMs = 0;
};

struct IrAnalyzerViewCache {
    bool initialized = false;
    bool active = false;
    int level = -1;
    uint8_t bars = 255;
    uint16_t edgesPerSec = 65535;
    uint16_t lowPulsesPerSec = 65535;
    uint16_t framesPerSec = 65535;
    uint16_t noisePerSec = 65535;
    unsigned long totalFrames = 0xFFFFFFFF;
    unsigned long totalNoise = 0xFFFFFFFF;
    unsigned long totalLowPulses = 0xFFFFFFFF;
    IrAnalyzerSignalState signalState = IR_SIGNAL_IDLE;
    uint16_t lastRawCount = 65535;
    uint32_t lastFrameTotalUs = 0xFFFFFFFF;
    uint16_t lastHeaderUs = 65535;
    bool lastOverflow = false;
    uint16_t lastFrameGapMs = 65535;
    uint16_t lastAgeSec = 65535;
    String lastRawText;
};

static void prepareAnalyzerPins() {
    ledcDetachPin(IR_TX_PIN);
    pinMode(IR_TX_PIN, OUTPUT);
    digitalWrite(IR_TX_PIN, HIGH);
    pinMode(IR_RX_PIN, INPUT);
}

static void initAnalyzerState(IrAnalyzerState& state) {
    state = IrAnalyzerState();
    state.level = digitalRead(IR_RX_PIN);
    state.lastChangeUs = micros();
    state.lastEdgeUs = state.lastChangeUs;
    state.windowStartMs = millis();
}

static void drawAnalyzerFrame() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.drawFastHLine(0, 34, 320, TFT_WHITE);
    tft.drawFastHLine(0, 212, 320, TFT_WHITE);
    drawStringBig(10, 8, "IR ANALYZER", TFT_WHITE, 1);
    drawStringCustom(12, 44, "SIGNAL", TFT_CYAN, 1);
    drawStringCustom(12, 154, "LAST FRAME", TFT_CYAN, 1);
    drawStringCustom(10, 220, "OK: RETURN  UP/DN: RESET", TFT_WHITE, 1);
}

static void appendAnalyzerDuration(IrAnalyzerState& state, uint32_t duration) {
    if (state.rawCount < ANALYZER_RAW_MAX) {
        if (duration > 65535) duration = 65535;
        state.raw[state.rawCount++] = static_cast<uint16_t>(duration);
    } else {
        state.frameOverflow = true;
    }
}

static void recordAnalyzerNoise(IrAnalyzerState& state) {
    state.totalNoise++;
    state.windowNoise++;
    state.lastNoiseMs = millis();
}

static void finishAnalyzerFrame(IrAnalyzerState& state) {
    if (!state.frameActive) return;

    if (state.rawCount >= 4) {
        uint32_t total = 0;
        for (uint16_t i = 0; i < state.rawCount; i++) total += state.raw[i];

        state.totalFrames++;
        state.windowFrames++;
        unsigned long nowMs = millis();
        state.lastFrameGapMs = (state.lastFrameMs == 0)
            ? 0
            : static_cast<uint16_t>(
                  min<unsigned long>(9999, nowMs - state.lastFrameMs));
        state.lastFrameMs = nowMs;
        state.lastRawCount = state.rawCount;
        state.lastFrameTotalUs = total;
        state.lastHeaderUs = state.raw[0];
        state.lastOverflow = state.frameOverflow;
        state.lastRawPreviewCount = min<uint16_t>(state.rawCount,
                                                  ANALYZER_RAW_PREVIEW);
        for (uint8_t i = 0; i < state.lastRawPreviewCount; i++) {
            state.lastRawPreview[i] = state.raw[i];
        }
    } else {
        recordAnalyzerNoise(state);
    }

    state.frameActive = false;
    state.frameOverflow = false;
    state.rawCount = 0;
}

static void processAnalyzerSample(IrAnalyzerState& state) {
    uint32_t nowUs = micros();
    int level = digitalRead(IR_RX_PIN);

    if (level != state.level) {
        uint32_t duration = nowUs - state.lastChangeUs;
        int previousLevel = state.level;

        if (duration >= ANALYZER_MIN_PULSE_US) {
            state.totalEdges++;
            state.windowEdges++;
            state.lastActivityMs = millis();
            state.lastEdgeUs = nowUs;

            if (previousLevel == LOW) {
                state.totalLowPulses++;
                state.windowLowPulses++;
            }

            if (!state.frameActive &&
                previousLevel == LOW &&
                duration >= ANALYZER_START_MIN_US &&
                duration <= ANALYZER_START_MAX_US) {
                state.frameActive = true;
                state.frameStartUs = nowUs - duration;
                state.rawCount = 0;
                state.frameOverflow = false;
                appendAnalyzerDuration(state, duration);
            } else if (state.frameActive) {
                appendAnalyzerDuration(state, duration);
            } else if (previousLevel == LOW) {
                recordAnalyzerNoise(state);
            }
        }

        state.level = level;
        state.lastChangeUs = nowUs;
    }

    if (state.frameActive &&
        ((nowUs - state.lastEdgeUs) > ANALYZER_FRAME_GAP_US ||
         (nowUs - state.frameStartUs) > ANALYZER_FRAME_MAX_US)) {
        finishAnalyzerFrame(state);
    }

    unsigned long nowMs = millis();
    unsigned long elapsed = nowMs - state.windowStartMs;
    if (elapsed >= 1000) {
        state.edgesPerSec = static_cast<uint16_t>(
            min<unsigned long>(999, (state.windowEdges * 1000UL) / elapsed));
        state.lowPulsesPerSec = static_cast<uint16_t>(
            min<unsigned long>(999, (state.windowLowPulses * 1000UL) / elapsed));
        state.framesPerSec = static_cast<uint16_t>(
            min<unsigned long>(99, (state.windowFrames * 1000UL) / elapsed));
        state.noisePerSec = static_cast<uint16_t>(
            min<unsigned long>(999, (state.windowNoise * 1000UL) / elapsed));

        state.windowEdges = 0;
        state.windowLowPulses = 0;
        state.windowFrames = 0;
        state.windowNoise = 0;
        state.windowStartMs = nowMs;
    }
}

static IrAnalyzerSignalState signalStateFor(const IrAnalyzerState& state) {
    unsigned long nowMs = millis();
    bool recentFrame = state.lastFrameMs != 0 &&
                       (nowMs - state.lastFrameMs) < ANALYZER_ACTIVE_MS;
    bool recentNoise = state.lastNoiseMs != 0 &&
                       (nowMs - state.lastNoiseMs) < ANALYZER_ACTIVE_MS;
    bool repeated = recentFrame &&
                    ((state.lastFrameGapMs > 0 &&
                      state.lastFrameGapMs < ANALYZER_REPEAT_MS) ||
                     state.framesPerSec >= 2);
    bool noisy = recentNoise &&
                 (state.framesPerSec == 0 || state.noisePerSec > 8);

    if (noisy && !recentFrame) return IR_SIGNAL_NOISE;
    if (repeated) return IR_SIGNAL_REPEAT;
    if (recentFrame) return IR_SIGNAL_FRAME;
    if (noisy) return IR_SIGNAL_NOISE;
    return IR_SIGNAL_IDLE;
}

static uint8_t activityBarsFor(IrAnalyzerSignalState signalState) {
    switch (signalState) {
        case IR_SIGNAL_FRAME:  return 4;
        case IR_SIGNAL_REPEAT: return 7;
        case IR_SIGNAL_NOISE:  return 10;
        case IR_SIGNAL_IDLE:
        default:               return 0;
    }
}

static uint16_t barColorFor(IrAnalyzerSignalState signalState) {
    switch (signalState) {
        case IR_SIGNAL_FRAME:  return TFT_GREEN;
        case IR_SIGNAL_REPEAT: return TFT_YELLOW;
        case IR_SIGNAL_NOISE:  return TFT_RED;
        case IR_SIGNAL_IDLE:
        default:               return TFT_DARKGREY;
    }
}

static String signalLabelFor(IrAnalyzerSignalState signalState) {
    switch (signalState) {
        case IR_SIGNAL_FRAME:  return "FRAME";
        case IR_SIGNAL_REPEAT: return "REPEAT";
        case IR_SIGNAL_NOISE:  return "NOISE";
        case IR_SIGNAL_IDLE:
        default:               return "IDLE";
    }
}

static void drawBar(int x, int y, uint8_t bars,
                    IrAnalyzerSignalState signalState) {
    uint16_t color = barColorFor(signalState);
    for (uint8_t i = 0; i < 10; i++) {
        int bx = x + i * 18;
        tft.drawRect(bx, y, 13, 14, TFT_WHITE);
        if (i < bars) tft.fillRect(bx + 2, y + 2, 9, 10, color);
        else          tft.fillRect(bx + 2, y + 2, 9, 10, TFT_BLACK);
    }
}

static String rawPreviewText(const IrAnalyzerState& state) {
    if (state.lastRawPreviewCount == 0) return "-";

    String text;
    for (uint8_t i = 0; i < state.lastRawPreviewCount; i++) {
        if (i) text += ",";
        text += String(state.lastRawPreview[i]);
    }
    if (state.lastRawCount > state.lastRawPreviewCount) text += ",..";
    return text;
}

static void drawAnalyzerLive(const IrAnalyzerState& state,
                             IrAnalyzerViewCache& view) {
    IrAnalyzerSignalState signalState = signalStateFor(state);
    bool active = signalState != IR_SIGNAL_IDLE;
    uint8_t bars = activityBarsFor(signalState);
    int level = digitalRead(IR_RX_PIN);

    if (!view.initialized ||
        view.active != active ||
        view.signalState != signalState) {
        tft.fillRect(80, 42, 118, 20, TFT_BLACK);
        drawStringCustom(82, 44, signalLabelFor(signalState),
                         barColorFor(signalState), 2);
        view.active = active;
        view.signalState = signalState;
    }

    if (!view.initialized || view.level != level) {
        tft.fillRect(208, 44, 72, 14, TFT_BLACK);
        drawStringCustom(210, 46, "LVL " + String(level), TFT_WHITE, 1);
        view.level = level;
    }

    if (!view.initialized ||
        view.bars != bars ||
        view.signalState != signalState) {
        drawBar(18, 70, bars, signalState);
        view.bars = bars;
    }

    if (!view.initialized ||
        view.edgesPerSec != state.edgesPerSec ||
        view.lowPulsesPerSec != state.lowPulsesPerSec ||
        view.framesPerSec != state.framesPerSec ||
        view.noisePerSec != state.noisePerSec ||
        view.totalFrames != state.totalFrames ||
        view.totalNoise != state.totalNoise ||
        view.totalLowPulses != state.totalLowPulses) {
        tft.fillRect(12, 98, 286, 50, TFT_BLACK);
        drawStringCustom(12, 100, "EDGES/S : " + String(state.edgesPerSec),
                         TFT_WHITE, 1);
        drawStringCustom(154, 100, "LOW/S: " + String(state.lowPulsesPerSec),
                         TFT_WHITE, 1);
        drawStringCustom(12, 116, "FRAMES  : " + String(state.totalFrames),
                         TFT_YELLOW, 1);
        drawStringCustom(154, 116, "FPS  : " + String(state.framesPerSec),
                         TFT_YELLOW, 1);
        drawStringCustom(12, 132, "NOISE   : " + String(state.totalNoise),
                         TFT_DARKGREY, 1);
        drawStringCustom(154, 132, "N/S  : " + String(state.noisePerSec),
                         TFT_DARKGREY, 1);

        view.edgesPerSec = state.edgesPerSec;
        view.lowPulsesPerSec = state.lowPulsesPerSec;
        view.framesPerSec = state.framesPerSec;
        view.noisePerSec = state.noisePerSec;
        view.totalFrames = state.totalFrames;
        view.totalNoise = state.totalNoise;
        view.totalLowPulses = state.totalLowPulses;
    }

    if (state.lastRawCount == 0) {
        if (!view.initialized || view.lastRawCount != 0) {
            tft.fillRect(18, 168, 292, 42, TFT_BLACK);
            drawStringCustom(20, 172, "Waiting for valid IR frame...",
                             TFT_WHITE, 1);
            view.lastRawCount = 0;
        }
        view.initialized = true;
        return;
    }

    uint16_t ageSec = static_cast<uint16_t>(
        min<unsigned long>(999, (millis() - state.lastFrameMs) / 1000UL));
    String rawText = rawPreviewText(state);

    if (!view.initialized ||
        view.lastRawCount != state.lastRawCount ||
        view.lastFrameTotalUs != state.lastFrameTotalUs ||
        view.lastHeaderUs != state.lastHeaderUs ||
        view.lastOverflow != state.lastOverflow ||
        view.lastFrameGapMs != state.lastFrameGapMs ||
        view.lastAgeSec != ageSec ||
        view.lastRawText != rawText) {
        tft.fillRect(18, 168, 292, 42, TFT_BLACK);
        drawStringCustom(20, 170, "COUNT: " + String(state.lastRawCount) +
                         "  TOTAL: " +
                         String(state.lastFrameTotalUs / 1000.0f, 1) +
                         " ms", TFT_WHITE, 1);
        drawStringCustom(20, 186, "HEAD: " + String(state.lastHeaderUs) +
                         " us  GAP: " + String(state.lastFrameGapMs) + " ms",
                         state.lastOverflow ? TFT_RED : TFT_WHITE, 1);
        drawStringFit(20, 200, "AGE " + String(ageSec) + "s RAW: " + rawText,
                      TFT_WHITE, 288, 1);

        view.lastRawCount = state.lastRawCount;
        view.lastFrameTotalUs = state.lastFrameTotalUs;
        view.lastHeaderUs = state.lastHeaderUs;
        view.lastOverflow = state.lastOverflow;
        view.lastFrameGapMs = state.lastFrameGapMs;
        view.lastAgeSec = ageSec;
        view.lastRawText = rawText;
    }

    view.initialized = true;
}

void runIrAnalyzer() {
    prepareAnalyzerPins();

    IrAnalyzerState state;
    IrAnalyzerViewCache view;
    initAnalyzerState(state);
    drawAnalyzerFrame();

    unsigned long lastDraw = 0;
    while (true) {
        processAnalyzerSample(state);

        if (millis() - lastDraw >= ANALYZER_DRAW_MS) {
            drawAnalyzerLive(state, view);
            lastDraw = millis();
        }

        if (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW) {
            while (digitalRead(BTN_UP) == LOW ||
                   digitalRead(BTN_DOWN) == LOW) delay(5);
            initAnalyzerState(state);
            view = IrAnalyzerViewCache();
            drawAnalyzerFrame();
            lastDraw = 0;
        }

        if (digitalRead(BTN_OK) == LOW) {
            while (digitalRead(BTN_OK) == LOW) delay(5);
            delay(60);
            return;
        }

        delayMicroseconds(80);
    }
}
