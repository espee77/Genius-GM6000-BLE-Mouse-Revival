#pragma once
#include <Arduino.h>

void powerBegin();
void powerWake(const char *reason);
void powerRecordActivity();
void powerUpdate();

bool powerIsIdleSleep();
bool powerCanSendMouseReport();

// IR power policy:
// true  = ball mouse mode, IR may be on while awake
// false = air mouse mode, IR stays off
void powerSetIrAllowedByMode(bool allowed);

// Immediately save state and enter System OFF because the battery is critical.
void powerEnterCriticalShutdown();
