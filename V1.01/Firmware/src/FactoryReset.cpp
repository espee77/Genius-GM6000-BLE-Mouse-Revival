#include "FactoryReset.h"
#include "Pins.h"
#include "StatusLeds.h"
#include "BleMouse.h"

#include <Arduino.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <bluefruit.h>

namespace {
constexpr uint32_t FACTORY_RESET_HOLD_MS = 5000UL;
constexpr uint32_t FACTORY_RESET_POLL_MS = 10UL;

bool allMouseButtonsPressed()
{
    return digitalRead(BTN_LEFT) == LOW &&
           digitalRead(BTN_MIDDLE) == LOW &&
           digitalRead(BTN_RIGHT) == LOW;
}
}

bool factoryResetRequestedAtBoot()
{
    if (!allMouseButtonsPressed()) {
        return false;
    }

    const uint32_t startedAt = millis();

    // Alternate the external LEDs while the user keeps all three buttons held.
    while (millis() - startedAt < FACTORY_RESET_HOLD_MS) {
        if (!allMouseButtonsPressed()) {
            allExternalLedsOff();
            return false;
        }

        const bool phase = ((millis() - startedAt) / 250UL) & 1UL;
        setRedLed(phase);
        setBlueLed(!phase);
        delay(FACTORY_RESET_POLL_MS);
    }

    allExternalLedsOff();
    return true;
}

[[noreturn]] void factoryResetExecute()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("GM-6000 FACTORY RESET");
    Serial.println("Clearing BLE bonds...");

    // Bluefruit is already initialized by mouseOutputBegin(). This removes all
    // peer/bond records stored by the BLE stack.
    bleMouseDisconnectIfConnected();
    Bluefruit.Periph.clearBonds();

    Serial.println("Formatting InternalFS...");
    const bool fsMounted = InternalFS.begin();
    const bool fsFormatted = fsMounted && InternalFS.format();

    // InternalFS contains all application-persistent files in this firmware,
    // including usage, battery and system/event statistics and any saved
    // settings added to that filesystem.
    if (fsFormatted) {
        Serial.println("InternalFS format complete.");
    } else {
        Serial.println("WARNING: InternalFS format failed.");
    }

    // The BLE device name is compiled into the firmware. The nRF52840 hardware
    // identity is factory-programmed and intentionally cannot be reset here.
    Serial.println("Saved runtime data cleared.");
    Serial.println("Rebooting...");

    setRedLed(true);
    setBlueLed(true);
    delay(750);
    allExternalLedsOff();
    delay(100);

    NVIC_SystemReset();
    while (true) { }
}
