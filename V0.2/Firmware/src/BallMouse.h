#pragma once
#include <Arduino.h>
#include "MouseReport.h"

void ballMouseBegin();
MouseReportData ballMouseUpdate(uint8_t buttons, bool &activity);
