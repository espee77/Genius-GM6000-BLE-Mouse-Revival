#pragma once
#include <Arduino.h>
#include <bluefruit.h>
#include "MouseReport.h"

extern BLEDis bledis;
extern BLEHidAdafruit blehid;
extern BLEBas blebas;

void bleMouseBegin();
void bleMouseStartAdvertising();
void bleMouseSetAdvertisingAllowed(bool allowed);
bool bleMouseConnected();
bool bleMouseInputEnabled();
bool bleMouseIdleProfileRequested();

bool bleMouseTrySendReport(const MouseReportData &report);
void bleMouseBatteryWrite(uint8_t percent);
void bleMouseBatteryPowerStateWrite(uint8_t state);
void bleMouseDisconnectIfConnected();

void bleMouseUseActiveProfile();
void bleMouseUseIdleProfile();
void bleMouseSetInputEnabled(bool enabled);
