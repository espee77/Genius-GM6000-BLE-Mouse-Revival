#include "BallMouse.h"
#include "Encoders.h"
#include "Config.h"
#include <bluefruit.h>

void ballMouseBegin() {}

static float scrollAccumulator = 0.0f;
static float scrollVelocity = 0.0f;
static uint32_t lastScrollMs = 0;

static float scrollGainFromSensitivity() {
    switch (SCROLL_SENSITIVITY) {
        case 0:  return 0.04f;
        case 1:  return 0.06f;
        case 2:  return 0.08f;
        case 3:  return 0.10f;
        case 4:  return 0.14f;
        case 5:  return 0.18f;
        case 6:  return 0.25f;
        case 7:  return 0.35f;
        case 8:  return 0.50f;
        case 9:  return 0.75f;
        default: return 1.00f;
    }
}

static int8_t smoothScrollFromRawY(long rawY) {
    uint32_t now = millis();

    if (lastScrollMs == 0) {
        lastScrollMs = now;
    }

    uint32_t dt = now - lastScrollMs;
    lastScrollMs = now;

    if (dt > 30) dt = 30;

    float gain = scrollGainFromSensitivity() * SCROLL_SPEED;

    // Nieuwe balbeweging voegt scroll-snelheid toe
    scrollVelocity += (float)rawY * gain;

    // Demping / momentum
    // Lager = directer
    // Hoger = soepeler en langer uitrollen
    scrollVelocity *= 0.82f;

    // Kleine restjes stoppen
    if (rawY == 0 && scrollVelocity > -0.03f && scrollVelocity < 0.03f) {
        scrollVelocity = 0.0f;
    }

    scrollAccumulator += scrollVelocity;

    int16_t wheel = (int16_t)scrollAccumulator;

    if (wheel > 127)  wheel = 127;
    if (wheel < -127) wheel = -127;

    scrollAccumulator -= wheel;

    return (int8_t)wheel;
}

MouseReportData ballMouseUpdate(uint8_t buttons, bool &activity) {
    long rawX;
    long rawY;
    encodersConsume(rawX, rawY);

    int8_t moveX = constrain(rawX * MOUSE_SPEED, -127, 127);
    int8_t moveY = constrain(rawY * MOUSE_SPEED, -127, 127);

    if (INVERT_X) moveX = -moveX;
    if (INVERT_Y) moveY = -moveY;

    int8_t wheel = 0;
    bool middleScrollMode = buttons & MOUSE_BUTTON_MIDDLE;

    if (middleScrollMode) {
        wheel = smoothScrollFromRawY(rawY);

        if (INVERT_SCROLL) {
            wheel = -wheel;
        }

        moveX = 0;
        moveY = 0;

        // Middelknop niet als normale klik doorgeven tijdens scroll-mode
        buttons &= ~MOUSE_BUTTON_MIDDLE;
    } else {
        // Geen scroll-mode: momentum netjes afbouwen/resetten
        scrollVelocity *= 0.70f;
        scrollAccumulator *= 0.70f;

        if (scrollVelocity > -0.03f && scrollVelocity < 0.03f) {
            scrollVelocity = 0.0f;
        }

        if (scrollAccumulator > -0.05f && scrollAccumulator < 0.05f) {
            scrollAccumulator = 0.0f;
        }

        lastScrollMs = 0;
    }

    MouseReportData report;
    report.buttons = buttons;
    report.x = moveX;
    report.y = moveY;
    report.wheel = wheel;
    report.pan = 0;

    activity = report.hasMotion();
    return report;
}