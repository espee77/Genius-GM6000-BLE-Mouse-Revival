#pragma once
#include <Arduino.h>
#include "MouseReport.h"

void mouseOutputBegin();
void mouseOutputUpdate();
bool mouseOutputConnected();
bool mouseOutputCanSend();
void mouseOutputSendReport(const MouseReportData &report);
