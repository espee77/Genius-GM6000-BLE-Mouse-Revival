#include "Buttons.h"
#include "Pins.h"
#include "Config.h"
#include <bluefruit.h>

static uint8_t stableButtons = 0;
static uint8_t previousButtonsState = 0;
static uint8_t lastRawButtons = 0;
static uint32_t lastChangeTime = 0;

static bool stableMultiHost = false;
static bool previousMultiHost = false;
static bool lastRawMultiHost = false;
static uint32_t lastMultiHostChangeTime = 0;

static uint8_t readButtonsRaw() {
    uint8_t buttons = 0;

    if (digitalRead(BTN_LEFT) == LOW) buttons |= MOUSE_BUTTON_LEFT;
    if (digitalRead(BTN_RIGHT) == LOW) buttons |= MOUSE_BUTTON_RIGHT;
    if (digitalRead(BTN_MIDDLE) == LOW) buttons |= MOUSE_BUTTON_MIDDLE;

    return buttons;
}

static bool readMultiHostRaw() {
    return digitalRead(BTN_MULTI_HOST) == LOW;
}

void buttonsBegin() {
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_MIDDLE, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_MULTI_HOST, INPUT_PULLUP);

    stableButtons = readButtonsRaw();
    previousButtonsState = stableButtons;
    lastRawButtons = stableButtons;

    stableMultiHost = readMultiHostRaw();
    previousMultiHost = stableMultiHost;
    lastRawMultiHost = stableMultiHost;
}

void buttonsUpdate() {
    previousButtonsState = stableButtons;
    previousMultiHost = stableMultiHost;

    uint8_t rawButtons = readButtonsRaw();
    if (rawButtons != lastRawButtons) {
        lastRawButtons = rawButtons;
        lastChangeTime = millis();
    }
    if (millis() - lastChangeTime >= BUTTON_DEBOUNCE_MS) {
        stableButtons = rawButtons;
    }

    bool rawMulti = readMultiHostRaw();
    if (rawMulti != lastRawMultiHost) {
        lastRawMultiHost = rawMulti;
        lastMultiHostChangeTime = millis();
    }
    if (millis() - lastMultiHostChangeTime >= BUTTON_DEBOUNCE_MS) {
        stableMultiHost = rawMulti;
    }
}

uint8_t buttonsCurrent() { return stableButtons; }
uint8_t buttonsPrevious() { return previousButtonsState; }
bool buttonsChanged() { return stableButtons != previousButtonsState; }

bool buttonLeftNewPress() {
    return (stableButtons & MOUSE_BUTTON_LEFT) && !(previousButtonsState & MOUSE_BUTTON_LEFT);
}

bool buttonRightNewPress() {
    return (stableButtons & MOUSE_BUTTON_RIGHT) && !(previousButtonsState & MOUSE_BUTTON_RIGHT);
}

bool buttonMiddleNewPress() {
    return (stableButtons & MOUSE_BUTTON_MIDDLE) && !(previousButtonsState & MOUSE_BUTTON_MIDDLE);
}

bool multiHostNewPress() {
    return stableMultiHost && !previousMultiHost;
}
