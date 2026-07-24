#pragma once
#include <Arduino.h>

enum class BatteryChargeStatus : uint8_t {
    OnBattery = 0,
    Charging,
    FullyCharged
};

void batteryBegin();
void batteryUpdate();
void batteryUpdateNow();

uint8_t batteryPercent();
float batteryVoltage();
uint32_t batteryRaw();

bool batteryUsbPowerPresent();
bool batteryIsCharging();
bool batteryIsFullyCharged();
const char* batteryChargeStatusText();
