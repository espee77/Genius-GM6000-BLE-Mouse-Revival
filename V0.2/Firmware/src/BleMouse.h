#pragma once
#include <Arduino.h>
#include <bluefruit.h>
#include "MouseReport.h"

extern BLEDis bledis;
extern BLEHidAdafruit blehid;
extern BLEBas blebas;

void bleMouseBegin();
void bleMouseStartAdvertising();
bool bleMouseConnected();
void bleMouseSendReport(const MouseReportData &report);
void bleMouseBatteryWrite(uint8_t percent);
void bleMouseDisconnectIfConnected();
