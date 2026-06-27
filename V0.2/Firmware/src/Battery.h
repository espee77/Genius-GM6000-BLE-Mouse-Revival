#pragma once
#include <Arduino.h>

void batteryBegin();
void batteryUpdate();
void batteryUpdateNow();
uint8_t batteryPercent();
float batteryVoltage();
uint32_t batteryRaw();
