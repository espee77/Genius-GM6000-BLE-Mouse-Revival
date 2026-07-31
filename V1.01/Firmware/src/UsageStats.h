#pragma once
#include <Arduino.h>

void usageStatsBegin();
void usageStatsUpdate(bool idle, bool usbMode, bool irOn, bool bleConnected);
void usageStatsProcessSerial();
void usageStatsSave();
void usageStatsPrint();
void usageStatsReset();

void usageStatsRecordWake(const char *reason, bool fromIdle);
void usageStatsRecordButtonPress(const char *buttonName);
void usageStatsRecordEncoderSteps(long x, long y);
void usageStatsRecordMouseReport(bool usbReport);
void usageStatsRecordBleConnect();
void usageStatsRecordBleDisconnect(uint8_t reason);
void usageStatsRecordOutputModeChange(bool usbMode);
void usageStatsRecordIdleEntry();
void usageStatsRecordBattery(float voltage, uint8_t percent, bool charging, bool fullyCharged);

// True after the single shared InternalFS initialization succeeded.
bool usageStatsStorageReady();
