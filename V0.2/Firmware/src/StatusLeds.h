#pragma once
#include <Arduino.h>

void statusLedsBegin();
void keepXiaoOnboardLedsOff();
void setBlueLed(bool on);
void setRedLed(bool on);
void allExternalLedsOff();
void statusLedsUpdate(bool idleSleepMode, bool bleConnected, bool mouseOnBack, uint8_t batteryPercent);
