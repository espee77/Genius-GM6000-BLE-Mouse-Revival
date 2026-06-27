#pragma once
#include <Arduino.h>

void powerBegin();
void powerWake(const char *reason);
void powerRecordActivity();
void powerUpdate();
bool powerIsIdleSleep();
bool powerCanSendMouseReport();
