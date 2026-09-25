#include "LabTestEngine.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "WifiCore.h"
#include "Neopixel.h"
#include <WiFi.h>

extern DisplayTFT tft;

static LabTestReport report = {
    LAB_TEST_IDLE,
    "LAB BENCHMARK",
    "NO TARGET SELECTED",
    "SELECT FROM WIFI SCAN",
    "DUAL-BAND READY",
    0
};

static TargetLabInfo labTarget = { "Sample-AP", "00:11:22:33:44:55", 36, -65, 0 };
static bool hasTarget = true;

void labTestPrepare(const TargetLabInfo &target) {
    report.attempts++;
    strncpy(report.title, "LAB TEST C5", sizeof(report.title));
    snprintf(report.line1, sizeof(report.line1), "CH %u [%s] RSSI %ld",
             target.channel, is5GHzChannel(target.channel) ? "5GHz" : "2.4GHz", (long)target.rssi);
    report.state = LAB_TEST_READY;
    strncpy(report.line2, "Target Prepared", sizeof(report.line2));
    strncpy(report.line3, "Press OK to Run Test", sizeof(report.line3));
}

void labTestEvaluate(const TargetLabInfo &target, bool found, const LabStatsData &stats) {
    snprintf(report.line1, sizeof(report.line1), "CH %u [%s] RSSI %ld",
             target.channel, is5GHzChannel(target.channel) ? "5G" : "2.4G", (long)target.rssi);
    if (!found) {
        report.state = LAB_TEST_MEASURED;
        strncpy(report.line2, "Target Lost / Offline", sizeof(report.line2));
        uint8_t miss = stats.samples > 0 ? (stats.missed * 100UL / stats.samples) : 0;
        snprintf(report.line3, sizeof(report.line3), "Packet Loss: %u%%", miss);
    } else {
        report.state = LAB_TEST_MEASURED;
        snprintf(report.line2, sizeof(report.line2), "Samples: %u / Recv: %u", stats.samples, stats.found);
        snprintf(report.line3, sizeof(report.line3), "Avg RSSI: %ld dBm", (long)labStatsAverageRssi());
    }
}

void labTestReset() {
    report.state = LAB_TEST_IDLE;
    strncpy(report.line1, "Target Reset", sizeof(report.line1));
    strncpy(report.line2, "Ready for Next Test", sizeof(report.line2));
    strncpy(report.line3, "Select AP to begin", sizeof(report.line3));
    report.attempts = 0;
}

const LabTestReport &labTestGetReport() {
    return report;
}

void runLabTestEngine() {
    tft.fillScreen(TFT_BLACK);
    bool testing = false;
    unsigned long lastSample = 0;

    labStatsReset(hasTarget ? labTarget.bssid : nullptr);
    if (hasTarget) {
        labTestPrepare(labTarget);
    }

    while (true) {
        if (isBackPressed()) {
            flushNavInput(150);
            return;
        }

        if (isEnterPressed()) {
            flushNavInput(150);
            testing = !testing;
            clickTone();
            if (testing) {
                neopixelActivity();
            }
        }

        unsigned long now = millis();
        if (testing && (now - lastSample > 600)) {
            lastSample = now;
            // Quick scan sample on channel
            wifiCoreSetChannel(labTarget.channel);
            int n = WiFi.scanNetworks(false, true, false, 80, labTarget.channel);
            bool found = false;
            if (n > 0) {
                for (int i = 0; i < n; i++) {
                    if (WiFi.BSSIDstr(i).equalsIgnoreCase(labTarget.bssid) ||
                        WiFi.SSID(i).equalsIgnoreCase(labTarget.ssid)) {
                        found = true;
                        labTarget.rssi = WiFi.RSSI(i);
                        break;
                    }
                }
            }
            labStatsAdd(found, &labTarget);
            labTestEvaluate(labTarget, found, labStatsGet());
            WiFi.scanDelete();
            neopixelActivity();
        }

        // Render Ghostbox Lab Test HUD
        tft.fillRect(0, 0, 320, 24, 0x0010);
        drawStringCustom(8, 6, "GHOSTBOX 5G/2.4G LAB ENGINE", 0x07FF, 1);

        tft.fillRect(0, 26, 320, 180, TFT_BLACK);
        tft.drawRect(8, 30, 304, 170, 0x03EF);

        drawStringCustom(16, 38, report.title, TFT_YELLOW, 1);
        drawStringCustom(16, 56, report.line1, TFT_WHITE, 1);
        drawStringCustom(16, 74, report.line2, 0x07FF, 1);
        drawStringCustom(16, 92, report.line3, TFT_GREEN, 1);

        // Stats Box
        const LabStatsData &stats = labStatsGet();
        char buf[48];
        snprintf(buf, sizeof(buf), "Samples: %u  Loss: %u%%", stats.samples,
                 stats.samples > 0 ? (stats.missed * 100 / stats.samples) : 0);
        drawStringCustom(16, 120, buf, TFT_WHITE, 1);

        snprintf(buf, sizeof(buf), "Min: %ld dBm  Max: %ld dBm", (long)stats.minRssi, (long)stats.maxRssi);
        drawStringCustom(16, 138, buf, TFT_CYAN, 1);

        // Visual RSSI meter
        int barW = map(constrain(labTarget.rssi, -100, -30), -100, -30, 0, 280);
        tft.drawRect(16, 160, 284, 14, 0x5AEB);
        tft.fillRect(18, 162, barW, 10, testing ? 0x07E0 : 0x001F);

        // Footer
        tft.fillRect(0, 216, 320, 24, 0x0010);
        drawStringCustom(8, 222, testing ? "[OK] STOP TEST   [BACK] EXIT" : "[OK] START TEST   [BACK] EXIT", TFT_YELLOW, 1);

        delay(80);
    }
}
