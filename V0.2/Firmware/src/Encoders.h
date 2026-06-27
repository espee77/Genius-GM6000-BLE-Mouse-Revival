#pragma once
#include <Arduino.h>

void encodersBegin();
void encodersReset();
void encodersConsume(long &rawX, long &rawY);
