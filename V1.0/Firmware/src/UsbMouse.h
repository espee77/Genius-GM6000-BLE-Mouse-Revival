#pragma once
#include <Arduino.h>
#include "MouseReport.h"

void usbMouseBegin();
void usbMouseUpdate();
bool usbMouseHostMounted();
bool usbMouseModeChanged();
bool usbMouseTrySendReport(const MouseReportData &report);
