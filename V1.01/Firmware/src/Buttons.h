#pragma once
#include <Arduino.h>

void buttonsBegin();

// After System OFF wake, clear GPIO SENSE, capture the wake button, wait for
// release, and restart debounce from a clean released state. Returns only the
// three HID mouse-button bits that caused the wake.
uint8_t buttonsRecoverFromSystemOffWake();
void buttonsUpdate();
uint8_t buttonsCurrent();
bool buttonsChanged();
bool buttonLeftNewPress();
bool buttonRightNewPress();
bool buttonMiddleNewPress();
bool multiHostNewPress();
