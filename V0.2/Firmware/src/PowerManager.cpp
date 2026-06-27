#include <Arduino.h>
#include "PowerManager.h"
#include "Config.h"
#include "Pins.h"
#include "IrPower.h"
#include "StatusLeds.h"
#include "Encoders.h"
#include "BleMouse.h"
#include "nrf_gpio.h"
#include "nrf_soc.h"

extern const uint32_t g_ADigitalPinMap[];

static uint32_t lastActivityTime = 0;
static uint32_t lastWakeTime = 0;
static bool idleSleepMode = false;

static void configureButtonWakePin(uint32_t arduinoPin) {
    uint32_t nrfPin = g_ADigitalPinMap[arduinoPin];
    nrf_gpio_cfg_sense_input(nrfPin, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
}

static void enterDeepSleep() {
#if DEBUG_ENABLED && DEBUG_SLEEP
    Serial.println("Entering DEEP SLEEP");
    Serial.flush();
    delay(100);
#endif

    allExternalLedsOff();
    setIrLeds(false);
    digitalWrite(READ_BAT_ENABLE_PIN, HIGH);

    bleMouseDisconnectIfConnected();

    configureButtonWakePin(BTN_LEFT);
    configureButtonWakePin(BTN_RIGHT);
    configureButtonWakePin(BTN_MIDDLE);
    configureButtonWakePin(BTN_MULTI_HOST);

    delay(20);
    sd_power_system_off();

    while (true) {
        delay(1000);
    }
}

static void enterIdleSleep() {
    if (!idleSleepMode) {
        idleSleepMode = true;

#if DEBUG_ENABLED && DEBUG_SLEEP
        Serial.println("Entering IDLE SLEEP");
#endif

        setIrLeds(false);
        digitalWrite(READ_BAT_ENABLE_PIN, HIGH);
        allExternalLedsOff();

        // Oude encoder-delta's weggooien bij het ingaan van idle sleep.
        // Dit voorkomt dat restpulsen direct "MOVEMENT" veroorzaken.
        encodersReset();
    }
}

void powerBegin() {
    lastActivityTime = millis();
    lastWakeTime = 0;
    idleSleepMode = false;

#if DEBUG_ENABLED && DEBUG_SLEEP
    Serial.println("PowerManager started");
#endif
}

void powerWake(const char *reason) {
#if DEBUG_ENABLED && DEBUG_SLEEP
    Serial.print("Wake/activity: ");
    Serial.println(reason);
#else
    (void)reason;
#endif

    if (idleSleepMode) {
        setIrLeds(true);
        delay(POST_WAKE_ENCODER_SETTLE_MS);
        encodersReset();
        lastWakeTime = millis();
    }

    idleSleepMode = false;
    lastActivityTime = millis();

    keepXiaoOnboardLedsOff();
}

void powerRecordActivity() {
    lastActivityTime = millis();
}

void powerUpdate() {
    uint32_t elapsed = millis() - lastActivityTime;

#if DEBUG_ENABLED && DEBUG_SLEEP
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint >= 1000) {
        lastPrint = millis();
        Serial.print("Sleep elapsed ms: ");
        Serial.println(elapsed);
    }
#endif

    if (elapsed > DEEP_SLEEP_TIMEOUT_MS) {
        enterDeepSleep();
    }

    if (elapsed > SLEEP_TIMEOUT_MS) {
        enterIdleSleep();
    }
}

bool powerIsIdleSleep() {
    return idleSleepMode;
}

bool powerCanSendMouseReport() {
    if (idleSleepMode) return false;
    if (lastWakeTime == 0) return true;

    return millis() - lastWakeTime >= POST_WAKE_ENCODER_SETTLE_MS;
}