#include <Arduino.h>
#include <string.h>
#include "PowerManager.h"
#include "Config.h"
#include "Pins.h"
#include "IrPower.h"
#include "StatusLeds.h"
#include "Encoders.h"
#include "BleMouse.h"
#include "UsbMouse.h"
#include "ImuManager.h"
#include "nrf_gpio.h"
#include "nrf_soc.h"
#include "UsageStats.h"
#include "SystemManager.h"

extern const uint32_t g_ADigitalPinMap[];

// Separate timers:
// - idle timer: controls the short idle sleep, reset by every wake/activity including IMU wake
// - deep sleep timer: controls System OFF, reset only by real user activity
static uint32_t lastIdleActivityTime = 0;
static uint32_t lastDeepSleepActivityTime = 0;
static uint32_t lastWakeTime = 0;
static bool idleSleepMode = false;

// true  = ball mouse mode: IR may be used when awake
// false = air mouse mode: IR must stay off
static bool irAllowedByMode = true;

static void updateIrPower()
{
    const bool irShouldBeOn =
        !idleSleepMode &&
        irAllowedByMode;

    setIrLeds(irShouldBeOn);
}

static void configureButtonWakePin(uint32_t arduinoPin)
{
    const uint32_t nrfPin =
        g_ADigitalPinMap[arduinoPin];

    nrf_gpio_cfg_sense_input(
        nrfPin,
        NRF_GPIO_PIN_PULLUP,
        NRF_GPIO_PIN_SENSE_LOW
    );
}

static void enterDeepSleep()
{
#if DEBUG_ENABLED && DEBUG_SLEEP
    Serial.println("Entering DEEP SLEEP");
    Serial.flush();
    delay(100);
#endif

    allExternalLedsOff();
    setIrLeds(false);
    digitalWrite(READ_BAT_ENABLE_PIN, HIGH);

    systemManagerRecordEvent("Deep sleep");
    systemManagerFlush();
    usageStatsSave();
    bleMouseDisconnectIfConnected();

    // Zorg dat de onboard XIAO RGB-led uit blijft tijdens System OFF.
    keepXiaoOnboardLedsOff();

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

static void enterIdleSleep()
{
    if (idleSleepMode) {
        return;
    }

    idleSleepMode = true;
    usageStatsRecordIdleEntry();

#if DEBUG_ENABLED && DEBUG_SLEEP
    Serial.println("Entering IDLE SLEEP");
#endif

    updateIrPower();

    // Large idle-power reductions:
    // - gyroscope off, accelerometer at 26 Hz
    // BLE stays on the fast 6-9 connection interval. A 500 ms idle
    // interval caused slow and choppy wake-up on Windows 11.
    imuSetIdleProfile();
    imuResetWakeBaseline();

    digitalWrite(READ_BAT_ENABLE_PIN, HIGH);
    allExternalLedsOff();

    // Oude encoder-delta's weggooien bij het ingaan van idle sleep.
    // Dit voorkomt dat restpulsen direct "MOVEMENT" veroorzaken.
    encodersReset();
}

void powerBegin()
{
    const uint32_t now = millis();

    lastIdleActivityTime = now;
    lastDeepSleepActivityTime = now;
    lastWakeTime = 0;
    idleSleepMode = false;
    irAllowedByMode = true;

    imuSetBallProfile();
    bleMouseUseActiveProfile();
    updateIrPower();

#if DEBUG_ENABLED && DEBUG_SLEEP
    Serial.println("PowerManager started");
#endif
}

void powerSetIrAllowedByMode(bool allowed)
{
    if (irAllowedByMode == allowed) {
        return;
    }

    irAllowedByMode = allowed;

#if DEBUG_ENABLED && DEBUG_SLEEP
    Serial.print("IR allowed by mode: ");
    Serial.println(
        allowed
            ? "YES (ball mouse)"
            : "NO (air mouse)"
    );
#endif

    if (!idleSleepMode) {
        if (irAllowedByMode) {
            imuSetBallProfile();
        } else {
            imuSetAirProfile();
        }
    }

    updateIrPower();
}

void powerWake(const char *reason)
{
#if DEBUG_ENABLED && DEBUG_SLEEP
    Serial.print("Wake/activity: ");
    Serial.println(reason);
#else
    (void)reason;
#endif

    const bool wasIdle = idleSleepMode;

    usageStatsRecordWake(
        reason,
        wasIdle
    );

    if (idleSleepMode) {
        idleSleepMode = false;

        // Restore the fast BLE profile and the correct IMU profile
        // before normal mouse reports resume.
        bleMouseUseActiveProfile();

        if (irAllowedByMode) {
            imuSetBallProfile();
        } else {
            imuSetAirProfile();
        }

        updateIrPower();

        if (irAllowedByMode) {
            delay(POST_WAKE_ENCODER_SETTLE_MS);
        }

        encodersReset();
        lastWakeTime = millis();
    } else {
        updateIrPower();
    }

    const uint32_t now = millis();

    // Every wake/activity, including IMU wake, should keep the mouse awake
    // for the short idle timeout. This prevents immediate re-entering idle.
    lastIdleActivityTime = now;

    // But an IMU wake from idle should not extend the 30-minute deep-sleep timer,
    // otherwise small IMU noise can keep the mouse out of System OFF forever.
    const bool imuWakeFromIdle =
        wasIdle &&
        reason &&
        strcmp(reason, "IMU") == 0;

    if (!imuWakeFromIdle) {
        lastDeepSleepActivityTime = now;
    }

    keepXiaoOnboardLedsOff();
}

void powerRecordActivity()
{
    const uint32_t now = millis();

    lastIdleActivityTime = now;
    lastDeepSleepActivityTime = now;
}

void powerUpdate()
{
    const uint32_t now = millis();

    /*
     * Een echte USB-host levert continu voeding en gebruikt de muis
     * als bedrade USB HID-muis.
     *
     * In deze situatie:
     * - geen idle sleep;
     * - geen deep sleep/System OFF;
     * - IR-leds blijven aan in ball-mousemodus;
     * - IR-leds blijven uit in air-mousemodus.
     *
     * Een USB-lader of powerbank telt niet als host, omdat
     * usbMouseHostMounted() daar false blijft.
     */
    if (usbMouseHostMounted()) {
        if (idleSleepMode) {
            powerWake("USB HOST");
        } else {
            // Zorg dat de correcte IR- en IMU-modus actief blijft.
            if (irAllowedByMode) {
                imuSetBallProfile();
            } else {
                imuSetAirProfile();
            }

            updateIrPower();
        }

        /*
         * Houd beide timers actueel. Hierdoor krijgt de muis na het
         * loskoppelen van USB opnieuw de volledige idle- en
         * deep-sleeptijd, in plaats van onmiddellijk te gaan slapen.
         */
        lastIdleActivityTime = now;
        lastDeepSleepActivityTime = now;

        return;
    }

    const uint32_t idleElapsed =
        now - lastIdleActivityTime;

    const uint32_t deepSleepElapsed =
        now - lastDeepSleepActivityTime;

#if DEBUG_ENABLED && DEBUG_SLEEP
    static uint32_t lastPrint = 0;

    if (now - lastPrint >= 1000) {
        lastPrint = now;

        Serial.print("Idle elapsed ms: ");
        Serial.print(idleElapsed);
        Serial.print(" | Deep sleep elapsed ms: ");
        Serial.println(deepSleepElapsed);
    }
#endif

    if (deepSleepElapsed > DEEP_SLEEP_TIMEOUT_MS) {
        enterDeepSleep();
    }

    if (idleElapsed > SLEEP_TIMEOUT_MS) {
        enterIdleSleep();
    }
}

bool powerIsIdleSleep()
{
    return idleSleepMode;
}

bool powerCanSendMouseReport()
{
    if (idleSleepMode) {
        return false;
    }

    if (lastWakeTime == 0) {
        return true;
    }

    return
        millis() - lastWakeTime >=
        POST_WAKE_ENCODER_SETTLE_MS;
}

void powerEnterCriticalShutdown()
{
    allExternalLedsOff();
    setIrLeds(false);
    digitalWrite(READ_BAT_ENABLE_PIN, HIGH);

    usageStatsSave();
    bleMouseDisconnectIfConnected();
    keepXiaoOnboardLedsOff();

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
