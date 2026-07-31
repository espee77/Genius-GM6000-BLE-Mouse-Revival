#include "StatusLeds.h"
#include "Pins.h"
#include "Config.h"

static void writeExternalLed(uint8_t pin, bool on) {
    if (EXTERNAL_LED_ACTIVE_LOW) {
        digitalWrite(pin, on ? LOW : HIGH);
    } else {
        digitalWrite(pin, on ? HIGH : LOW);
    }
}

void setBlueLed(bool on) {
    writeExternalLed(LED_BLE_BLUE, on);
}

void setRedLed(bool on) {
    writeExternalLed(LED_BAT_RED, on);
}

void allExternalLedsOff() {
    setBlueLed(false);
    setRedLed(false);
}

void keepXiaoOnboardLedsOff() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

#ifdef LED_RED
    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_RED, HIGH);
#endif
#ifdef LED_GREEN
    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_GREEN, HIGH);
#endif
#ifdef LED_BLUE
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_BLUE, HIGH);
#endif

    pinMode(12, OUTPUT);
    digitalWrite(12, HIGH);

#ifdef PIN_NEOPIXEL
    pinMode(PIN_NEOPIXEL, OUTPUT);
    digitalWrite(PIN_NEOPIXEL, LOW);
#endif
#ifdef PIN_NEOPIXEL_POWER
    pinMode(PIN_NEOPIXEL_POWER, OUTPUT);
    digitalWrite(PIN_NEOPIXEL_POWER, LOW);
#endif
}

void statusLedsBegin() {
    pinMode(LED_BLE_BLUE, OUTPUT);
    pinMode(LED_BAT_RED, OUTPUT);
    allExternalLedsOff();
    keepXiaoOnboardLedsOff();
}

void statusLedsUpdate(bool idleSleepMode, bool bleConnected, bool mouseOnBack, uint8_t batteryPercent) {
    if (idleSleepMode || !mouseOnBack) {
        allExternalLedsOff();
        return;
    }

    uint32_t now = millis();

    if (bleConnected) {
        setBlueLed(true);
    } else {
        bool blueOn = (now % BLUE_ADVERTISING_PULSE_INTERVAL_MS) < BLUE_ADVERTISING_PULSE_ON_MS;
        setBlueLed(blueOn);
    }

    uint8_t redBlinkCount = 0;
    if (batteryPercent <= 10) {
        redBlinkCount = 3;
    } else if (batteryPercent <= 20) {
        redBlinkCount = 1;
    }

    bool redOn = false;
    if (redBlinkCount > 0) {
        uint32_t phase = now % RED_BATTERY_STATUS_INTERVAL_MS;
        for (uint8_t i = 0; i < redBlinkCount; i++) {
            uint32_t blinkStart = i * (RED_BATTERY_BLINK_ON_MS + RED_BATTERY_BLINK_GAP_MS);
            if (phase >= blinkStart && phase < blinkStart + RED_BATTERY_BLINK_ON_MS) {
                redOn = true;
            }
        }
    }

    setRedLed(redOn);
}
