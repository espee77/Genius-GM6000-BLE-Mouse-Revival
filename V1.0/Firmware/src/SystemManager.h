#pragma once
#include <Arduino.h>

// Call as the first operation in setup(). Captures and clears the hardware
// reset reason before other modules can alter it.
void systemManagerEarlyBegin();

// Call after BLE, USB, battery, IMU and statistics have been initialized.
void systemManagerBegin(bool imuStartedSuccessfully);
void systemManagerUpdate();

// Low-frequency system events. Stored in a small persistent ring buffer.
void systemManagerRecordEvent(const char *eventText);

// Diagnostics and service commands.
void systemManagerPrintStatus();
void systemManagerPrintEventLog();
void systemManagerFlush();
void systemManagerClearEventLog();
void systemManagerPrintReport();
void systemManagerRequestBootloader();

const char *systemManagerResetCauseText();
bool systemManagerSelfTestPassed();
