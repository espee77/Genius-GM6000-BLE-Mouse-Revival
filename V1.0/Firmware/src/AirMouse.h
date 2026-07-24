#pragma once
#include <Arduino.h>
#include "MouseReport.h"

void airMouseBegin();
bool airMouseIsActive();
MouseReportData airMouseUpdate(uint8_t buttons, bool &activity);
void airMouseReset();
