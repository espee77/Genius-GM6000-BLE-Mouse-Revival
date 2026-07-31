#include "Buttons.h"
#include "Pins.h"
#include "Config.h"
#include <bluefruit.h>
#include "nrf_gpio.h"

extern const uint32_t g_ADigitalPinMap[];

struct ButtonDebounceState {
    uint8_t mask;
    bool stablePressed;
    uint32_t releaseStartTime;
};

static ButtonDebounceState mouseButtons[] = {
    {MOUSE_BUTTON_LEFT, false, 0},
    {MOUSE_BUTTON_RIGHT, false, 0},
    {MOUSE_BUTTON_MIDDLE, false, 0}
};

static uint8_t stableButtons = 0;
static uint8_t previousButtonsState = 0;

static bool stableMultiHost = false;
static bool previousMultiHost = false;
static bool lastRawMultiHost = false;
static uint32_t lastMultiHostChangeTime = 0;


static void configureNormalButtonInput(uint32_t arduinoPin) {
    const uint32_t nrfPin = g_ADigitalPinMap[arduinoPin];

    // nrf_gpio_cfg_input() sets SENSE to Disabled. This explicitly removes
    // the SENSE_LOW wake configuration installed before System OFF.
    nrf_gpio_cfg_input(nrfPin, NRF_GPIO_PIN_PULLUP);
}

static bool anyWakeButtonPressedRaw() {
    return digitalRead(BTN_LEFT) == LOW ||
           digitalRead(BTN_MIDDLE) == LOW ||
           digitalRead(BTN_RIGHT) == LOW ||
           digitalRead(BTN_MULTI_HOST) == LOW;
}

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

static void initializeMouseButtonStates(uint8_t rawButtons) {
    stableButtons = rawButtons;
    previousButtonsState = rawButtons;

    for (ButtonDebounceState &button : mouseButtons) {
        button.stablePressed = (rawButtons & button.mask) != 0;
        button.releaseStartTime = 0;
    }
}

void buttonsBegin() {
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_MIDDLE, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_MULTI_HOST, INPUT_PULLUP);

    initializeMouseButtonStates(readButtonsRaw());

    stableMultiHost = readMultiHostRaw();
    previousMultiHost = stableMultiHost;
    lastRawMultiHost = stableMultiHost;
    lastMultiHostChangeTime = millis();
}


uint8_t buttonsRecoverFromSystemOffWake() {
    // First remove the latched GPIO SENSE configuration from every wake pin.
    configureNormalButtonInput(BTN_LEFT);
    configureNormalButtonInput(BTN_MIDDLE);
    configureNormalButtonInput(BTN_RIGHT);
    configureNormalButtonInput(BTN_MULTI_HOST);

    delay(2);
    const uint8_t wakeMouseButtons = readButtonsRaw();

    // Do not initialize BLE HID while a System OFF wake pin is still held.
    // A battery cold boot never has this special GPIO/wake condition.
    while (anyWakeButtonPressedRaw()) {
        delay(5);
    }

    // Start debounce and change detection from a fully released state.
    buttonsBegin();
    return wakeMouseButtons;
}

void buttonsUpdate() {
    previousButtonsState = stableButtons;
    previousMultiHost = stableMultiHost;

    const uint32_t now = millis();
    const uint8_t rawButtons = readButtonsRaw();

    for (ButtonDebounceState &button : mouseButtons) {
        const bool rawPressed = (rawButtons & button.mask) != 0;

        if (rawPressed) {
            // A press is accepted immediately for the lowest possible click latency.
            // Any pending release is cancelled when the contact closes again.
            button.releaseStartTime = 0;
            if (!button.stablePressed) {
                button.stablePressed = true;
                stableButtons |= button.mask;
            }
        } else if (button.stablePressed) {
            // A release must remain stable long enough. This filters the short
            // open pulses produced by contact bounce after pressing a switch.
            if (button.releaseStartTime == 0) {
                button.releaseStartTime = now;
            } else if (now - button.releaseStartTime >= BUTTON_DEBOUNCE_MS) {
                button.stablePressed = false;
                button.releaseStartTime = 0;
                stableButtons &= static_cast<uint8_t>(~button.mask);
            }
        } else {
            button.releaseStartTime = 0;
        }
    }

    // Keep conventional symmetric debounce for the infrequently used
    // multi-host button; click latency is not important for this function.
    const bool rawMulti = readMultiHostRaw();
    if (rawMulti != lastRawMultiHost) {
        lastRawMultiHost = rawMulti;
        lastMultiHostChangeTime = now;
    }
    if (now - lastMultiHostChangeTime >= BUTTON_DEBOUNCE_MS) {
        stableMultiHost = rawMulti;
    }
}

uint8_t buttonsCurrent() { return stableButtons; }
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
