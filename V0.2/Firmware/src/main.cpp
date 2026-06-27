// GENIUS GM-6000 BLE MOUSE
// Modular PlatformIO version
// Ball mouse + middle-hold scroll + air mouse + light/deep sleep

#include <Arduino.h>
#include "Config.h"
#include "Pins.h"
#include "MouseReport.h"
#include "IrPower.h"
#include "StatusLeds.h"
#include "Encoders.h"
#include "Buttons.h"
#include "ImuManager.h"
#include "Battery.h"
#include "BleMouse.h"
#include "PowerManager.h"
#include "BallMouse.h"
#include "AirMouse.h"

static uint8_t lastReportedButtons = 0;
static bool wasAirMouseActive = false;

static void handleWakeSources(bool movementActivity)
{
    if (movementActivity && !powerIsIdleSleep()) {
        powerWake("MOVEMENT");
        powerRecordActivity();
    }

    if (buttonsChanged()) {
        if (buttonLeftNewPress()) powerWake("LEFT");
        if (buttonRightNewPress()) powerWake("RIGHT");
        if (buttonMiddleNewPress()) powerWake("MIDDLE");
        powerRecordActivity();
    }

    if (multiHostNewPress()) {
        powerWake("MULTI-HOST");
        powerRecordActivity();
    }
}

void setup()
{
#if DEBUG_ENABLED
    Serial.begin(115200);
    uint32_t serialStart = millis();
    while (!Serial && millis() - serialStart < 5000) {
        delay(10);
    }
#endif

    statusLedsBegin();
    irPowerBegin();
    buttonsBegin();
    batteryBegin();
    encodersBegin();

    imuBegin();

    airMouseBegin();
    ballMouseBegin();

    bleMouseBegin();
    batteryUpdateNow();
    powerBegin();

    lastReportedButtons = buttonsCurrent();
}

void loop()
{
    batteryUpdate();
    buttonsUpdate();
    imuUpdate();

    bool airActive = airMouseIsActive();

    if (airActive != wasAirMouseActive) {
        airMouseReset();
        encodersReset();
        wasAirMouseActive = airActive;
    }

    uint8_t rawButtons = buttonsCurrent();
    bool movementActivity = false;

    MouseReportData report;

    if (airActive) {
        report = airMouseUpdate(rawButtons, movementActivity);
    } else {
        report = ballMouseUpdate(rawButtons, movementActivity);
    }

    // IMU wake alleen tijdens idle sleep.
    // Deep sleep blijft alleen wakker worden via knoppen.
    static uint32_t lastImuWakePoll = 0;

    if (powerIsIdleSleep() &&
        millis() - lastImuWakePoll >= IMU_POLL_INTERVAL_MS)
    {
        lastImuWakePoll = millis();

#if DEBUG_ENABLED && DEBUG_IMU
        Serial.println("Checking IMU wake");
#endif

        if (imuDetectPhysicalMovement()) {
#if DEBUG_ENABLED && DEBUG_IMU
            Serial.println("IMU WAKE DETECTED");
#endif
            powerWake("IMU");
        }
    }

    handleWakeSources(movementActivity);

    powerUpdate();

    bool reportButtonChanged = report.buttons != lastReportedButtons;

    if (
        bleMouseConnected() &&
        powerCanSendMouseReport() &&
        (report.hasMotion() || reportButtonChanged)
    ) {
        bleMouseSendReport(report);
        lastReportedButtons = report.buttons;
    }

    static uint32_t lastStatusLedUpdate = 0;
    if (millis() - lastStatusLedUpdate >= STATUS_LED_UPDATE_INTERVAL_MS) {
        lastStatusLedUpdate = millis();

        statusLedsUpdate(
            powerIsIdleSleep(),
            bleMouseConnected(),
            imuIsMouseOnBack(),
            batteryPercent()
        );
    }

    delay(MAIN_LOOP_DELAY_MS);
}