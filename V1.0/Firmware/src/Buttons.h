#pragma once
#include <Arduino.h>

void buttonsBegin();
void buttonsUpdate();
uint8_t buttonsCurrent();
bool buttonsChanged();
bool buttonLeftNewPress();
bool buttonRightNewPress();
bool buttonMiddleNewPress();
bool multiHostNewPress();
