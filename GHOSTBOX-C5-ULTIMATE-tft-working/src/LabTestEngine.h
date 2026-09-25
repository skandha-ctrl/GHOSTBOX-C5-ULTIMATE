#pragma once

#include <Arduino.h>
#include "LabStats.h"

enum LabTestState {
    LAB_TEST_IDLE,
    LAB_TEST_READY,
    LAB_TEST_MEASURED,
    LAB_TEST_SIMULATED,
    LAB_TEST_BLOCKED
};

struct LabTestReport {
    LabTestState state;
    char title[24];
    char line1[32];
    char line2[32];
    char line3[32];
    uint16_t attempts;
};

void labTestPrepare(const TargetLabInfo &target);
void labTestEvaluate(const TargetLabInfo &target, bool found, const LabStatsData &stats);
void labTestReset();
const LabTestReport &labTestGetReport();
void runLabTestEngine();
