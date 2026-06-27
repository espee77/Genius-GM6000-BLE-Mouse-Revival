#pragma once
#include <Arduino.h>

struct MouseReportData {
    uint8_t buttons = 0;
    int8_t x = 0;
    int8_t y = 0;
    int8_t wheel = 0;
    int8_t pan = 0;

    bool hasMotion() const {
        return x != 0 || y != 0 || wheel != 0 || pan != 0;
    }
};
