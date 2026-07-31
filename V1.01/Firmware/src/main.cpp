// GENIUS GM-6000 BLE MOUSE
// Modular PlatformIO version
// Ball mouse + middle-hold scroll + air mouse + optimized idle/deep sleep

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
#include "UsbMouse.h"
#include "MouseOutput.h"
#include "PowerManager.h"
#include "BallMouse.h"
#include "AirMouse.h"
#include "EnergyProfiler.h"
#include "UsageStats.h"
#include "SystemManager.h"
#include "FactoryReset.h"

static uint8_t lastReportedButtons = 0;
static bool wasAirMouseActive = false;

// A mouse button that wakes the nRF52840 from System OFF is already held
// while setup() runs. Without this small queue that first press becomes the
// initial button state and is never sent as a HID click.
static uint8_t startupWakeButtons = 0;
static uint8_t startupWakeStage = 0; // 0=none, 1=send down, 2=send/reconcile up
static uint32_t startupWakeDownSentAt = 0;

static uint32_t lastBallImuPoll = 0;
static uint32_t lastAirImuPoll = 0;
static uint32_t lastImuWakePoll = 0;
static uint32_t lastStatusLedUpdate = 0;

static void handleButtonWakeSources()
{
    if (buttonsChanged()) {
        if (buttonLeftNewPress()) { usageStatsRecordButtonPress("LEFT"); powerWake("LEFT"); }
        if (buttonRightNewPress()) { usageStatsRecordButtonPress("RIGHT"); powerWake("RIGHT"); }
        if (buttonMiddleNewPress()) { usageStatsRecordButtonPress("MIDDLE"); powerWake("MIDDLE"); }
        powerRecordActivity();
    }

    if (multiHostNewPress()) {
        usageStatsRecordButtonPress("MULTI-HOST");
        powerWake("MULTI-HOST");
        powerRecordActivity();
    }
}

static void handleMovementWakeSource(bool movementActivity)
{
    if (movementActivity && !powerIsIdleSleep()) {
        powerWake("MOVEMENT");
        powerRecordActivity();
    }
}

static void sendImmediateButtonReportIfNeeded()
{
    if (!buttonsChanged()) {
        return;
    }

    const uint8_t currentButtons = buttonsCurrent();

    if (
        currentButtons != lastReportedButtons &&
        mouseOutputCanSend() &&
        powerCanSendMouseReport()
    ) {
        MouseReportData buttonReport{};
        buttonReport.buttons = currentButtons;
        mouseOutputSendReport(buttonReport);
        lastReportedButtons = currentButtons;
    }
}


static void serviceStartupWakeClick()
{
    if (startupWakeStage == 0 || startupWakeButtons == 0) {
        return;
    }

    if (!mouseOutputCanSend() || !powerCanSendMouseReport()) {
        return;
    }

    const uint8_t currentButtons = buttonsCurrent();

    if (startupWakeStage == 1) {
        // Preserve any buttons currently held and add the button that caused
        // the wake. This also works when the user released it before BLE had
        // finished reconnecting.
        MouseReportData report{};
        report.buttons = currentButtons | startupWakeButtons;
        mouseOutputSendReport(report);
        lastReportedButtons = report.buttons;
        startupWakeDownSentAt = millis();
        startupWakeStage = 2;
        return;
    }

    // Keep a genuine long press held. When it was released before the BLE
    // reconnection, create a short but valid release after the queued press.
    if ((currentButtons & startupWakeButtons) != 0) {
        return;
    }

    if (millis() - startupWakeDownSentAt < 20) {
        return;
    }

    if (lastReportedButtons & startupWakeButtons) {
        MouseReportData report{};
        report.buttons = currentButtons;
        mouseOutputSendReport(report);
        lastReportedButtons = report.buttons;
    }

    startupWakeButtons = 0;
    startupWakeStage = 0;
}

static void updateStatusLedsIfDue()
{
    const uint32_t now = millis();

    if (now - lastStatusLedUpdate < STATUS_LED_UPDATE_INTERVAL_MS) {
        return;
    }

    lastStatusLedUpdate = now;

    statusLedsUpdate(
        powerIsIdleSleep(),
        mouseOutputConnected(),
        imuIsMouseOnBack(),
        batteryPercent()
    );
}

static void updateImuForActiveMode()
{
    const uint32_t now = millis();

    if (wasAirMouseActive) {
        if (now - lastAirImuPoll >= IMU_AIR_POLL_INTERVAL_MS) {
            lastAirImuPoll = now;
            energyProfilerImuPoll();
            imuUpdate();
        }
    } else {
        if (now - lastBallImuPoll >= IMU_BALL_POLL_INTERVAL_MS) {
            lastBallImuPoll = now;
            energyProfilerImuPoll();
            imuUpdateAccelOnly();
        }
    }
}

static void handleAirMouseModeTransition(bool airActive)
{
    if (airActive == wasAirMouseActive) {
        return;
    }

    airMouseReset();
    encodersReset();
    wasAirMouseActive = airActive;

    // Air mouse uses the gyro, but does not need the IR LEDs or encoders.
    // Ball mode powers the gyro down and only polls the accelerometer at 26 Hz.
    powerSetIrAllowedByMode(!airActive);
    encodersSetEnabled(!airActive);

    // The profile switch refreshes the IMU data, so align the poll timers.
    lastBallImuPoll = millis();
    lastAirImuPoll = millis();
}

void setup()
{
    systemManagerEarlyBegin();
    // XIAO nRF52840 Sense: LiPo charge current set to 100 mA.
    pinMode(22, OUTPUT);
    digitalWrite(22, LOW);

#if DEBUG_ENABLED || ENERGY_PROFILER_ENABLED
    Serial.begin(115200);
#endif

#if DEBUG_ENABLED
    const uint32_t serialStart = millis();
    while (!Serial && millis() - serialStart < 5000) {
        delay(10);
    }

    Serial.print("Firmware: ");
    Serial.println(FIRMWARE_VERSION_STRING);
#endif

    statusLedsBegin();
    irPowerBegin();
    buttonsBegin();

    // Hold left + middle + right continuously for five seconds during boot to
    // request a full reset of all resettable persistent runtime data.
    const bool factoryResetRequested = factoryResetRequestedAtBoot();

    // A System OFF wake is not electrically identical to reconnecting the
    // battery. Remove GPIO SENSE_LOW, wait for the held wake button to be
    // released, then start all input and BLE services from a clean state.
    // Keep the original wake click queued so it can be sent after reconnect.
    if (systemManagerWasSystemOffWake()) {
        startupWakeButtons = buttonsRecoverFromSystemOffWake();
        startupWakeStage = startupWakeButtons ? 1 : 0;
    } else {
        startupWakeButtons = 0;
        startupWakeStage = 0;
    }

    encodersBegin();

    const bool imuStartedSuccessfully = imuBegin();

    airMouseBegin();
    ballMouseBegin();

    // Start BLE/USB first so the BLE Battery Service exists before
    // the first battery percentage is published.
    mouseOutputBegin();

    if (factoryResetRequested) {
        factoryResetExecute();
    }

    batteryBegin();
    powerBegin();
    usageStatsBegin();
    systemManagerBegin(imuStartedSuccessfully);

    const bool initialAirActive = airMouseIsActive();
    wasAirMouseActive = initialAirActive;

    powerSetIrAllowedByMode(!initialAirActive);
    encodersSetEnabled(!initialAirActive);

    // Start from released when a wake button is queued, otherwise the first
    // press would again look as if it had already been reported.
    lastReportedButtons = startupWakeStage ? 0 : buttonsCurrent();

    const uint32_t now = millis();
    lastBallImuPoll = now;
    lastAirImuPoll = now;
    lastImuWakePoll = now;
    lastStatusLedUpdate = now;

    energyProfilerBegin(usbMouseHostMounted());
}

void loop()
{
    energyProfilerLoopBegin();

    batteryUpdate();
    mouseOutputUpdate();
    systemManagerUpdate();
    usageStatsProcessSerial();
    usageStatsUpdate(
        powerIsIdleSleep(),
        usbMouseHostMounted(),
        irLedsAreOn(),
        bleMouseConnected()
    );

    if (usbMouseModeChanged()) {
        const bool usbMode = usbMouseHostMounted();
        lastReportedButtons = buttonsCurrent();
        energyProfilerUsbModeChanged(usbMode);
    }

    energyProfilerUpdate();

    buttonsUpdate();

    // Buttons are handled before IMU, encoder and mouse calculations.
    // This gives clicks priority and immediately offers the HID report to USB/BLE.
    handleButtonWakeSources();
    serviceStartupWakeClick();
    sendImmediateButtonReportIfNeeded();

    if (powerIsIdleSleep()) {
        const uint32_t now = millis();

        // During idle, only the 26 Hz accelerometer is running.
        // No gyro reads, ball processing or air-mouse processing occur.
        if (now - lastImuWakePoll >= IMU_POLL_INTERVAL_MS) {
            lastImuWakePoll = now;

#if DEBUG_ENABLED && DEBUG_IMU
            Serial.println("Checking IMU wake");
#endif

            energyProfilerImuPoll();
            if (imuDetectPhysicalMovement()) {
#if DEBUG_ENABLED && DEBUG_IMU
                Serial.println("IMU WAKE DETECTED");
#endif
                powerWake("IMU");
            }
        }

        powerUpdate();
        updateStatusLedsIfDue();

        if (powerIsIdleSleep()) {
            // In the Adafruit nRF52 core delay() yields to tickless FreeRTOS,
            // allowing the CPU to sleep between BLE and timer events.
            energyProfilerBeforeDelay(IDLE_LOOP_DELAY_MS);
            delay(IDLE_LOOP_DELAY_MS);
            return;
        }
    }

    updateImuForActiveMode();

    const bool airActive = airMouseIsActive();
    handleAirMouseModeTransition(airActive);

    const uint8_t rawButtons = buttonsCurrent();
    const bool stableButtonChanged = buttonsChanged();
    const bool ballWorkPending =
        !airActive && (stableButtonChanged || ballMouseHasPendingWork(rawButtons));

    bool movementActivity = false;
    bool reportWasBuilt = false;
    MouseReportData report;

    if (airActive) {
        // Air mode must remain time-driven because gyro filtering needs a fixed cadence.
        report = airMouseUpdate(rawButtons, movementActivity);
        reportWasBuilt = true;
    } else if (ballWorkPending) {
        // Ball mode is event-driven: only consume encoder data when an ISR,
        // button transition or remaining scroll momentum requires work.
        report = ballMouseUpdate(rawButtons, movementActivity);
        reportWasBuilt = true;
    }

    handleMovementWakeSource(movementActivity);
    powerUpdate();

    if (reportWasBuilt) {
        const bool reportButtonChanged =
            report.buttons != lastReportedButtons;

        if (
            mouseOutputCanSend() &&
            powerCanSendMouseReport() &&
            (report.hasMotion() || reportButtonChanged)
        ) {
            mouseOutputSendReport(report);
            lastReportedButtons = report.buttons;
        }
    }

    updateStatusLedsIfDue();

    // Tickless FreeRTOS sleeps the CPU during delay(). Keep the fast cadence
    // while work is active, but wake less often when the ball mouse is quiet.
    if (airActive) {
        energyProfilerBeforeDelay(MAIN_LOOP_DELAY_MS);
        delay(MAIN_LOOP_DELAY_MS);
    } else if (ballWorkPending) {
        energyProfilerBeforeDelay(BALL_ACTIVE_LOOP_DELAY_MS);
        delay(BALL_ACTIVE_LOOP_DELAY_MS);
    } else {
        energyProfilerBeforeDelay(BALL_QUIET_LOOP_DELAY_MS);
        delay(BALL_QUIET_LOOP_DELAY_MS);
    }
}
