#include "MouseOutput.h"
#include "Config.h"
#include "BleMouse.h"
#include "UsbMouse.h"
#include "AirMouse.h"
#include "Buttons.h"
#include <math.h>
#include "EnergyProfiler.h"
#include "UsageStats.h"
#include "SystemManager.h"

static bool wasUsbMode = false;
static bool wasAirMode = false;

// Fractional movement that did not yet fit in an integer HID report.
// This is essential for multipliers below 1.0, for example 0.30.
static float xScaleRemainder = 0.0f;
static float yScaleRemainder = 0.0f;

// BLE reports are offered immediately. Only data that Bluefruit could not
// accept is retained and retried. This removes the fixed 8 ms waiting period
// without creating an unbounded stream of successful queued reports.
static int16_t pendingBleX = 0;
static int16_t pendingBleY = 0;
static int16_t pendingBleWheel = 0;
static int16_t pendingBlePan = 0;
static uint8_t pendingBleButtons = 0;
static uint8_t lastSentBleButtons = 0;
static uint32_t lastBleRetryMs = 0;
static bool pendingBleReport = false;

// USB can briefly report not-ready even while mounted. Keep unsent data so
// short clicks and releases are not lost between TinyUSB polling moments.
static int16_t pendingUsbX = 0;
static int16_t pendingUsbY = 0;
static int16_t pendingUsbWheel = 0;
static int16_t pendingUsbPan = 0;
static uint8_t pendingUsbButtons = 0;
static uint8_t lastSentUsbButtons = 0;
static uint32_t lastUsbRetryMs = 0;
static bool pendingUsbReport = false;

static void resetScaleRemainders()
{
    xScaleRemainder = 0.0f;
    yScaleRemainder = 0.0f;
}

static void addSaturated(int16_t &target, int8_t value)
{
    int32_t sum = (int32_t)target + value;
    if (sum > 32767) sum = 32767;
    if (sum < -32768) sum = -32768;
    target = (int16_t)sum;
}

static int8_t peekClampedAxis(int16_t value)
{
    if (value > 127) return 127;
    if (value < -127) return -127;
    return (int8_t)value;
}

static void consumeAxis(int16_t &value, int8_t sent)
{
    value -= sent;
}

static void clearPendingBleReport()
{
    pendingBleX = 0;
    pendingBleY = 0;
    pendingBleWheel = 0;
    pendingBlePan = 0;
    pendingBleButtons = lastSentBleButtons;
    pendingBleReport = false;
}

static void clearPendingUsbReport()
{
    pendingUsbX = 0;
    pendingUsbY = 0;
    pendingUsbWheel = 0;
    pendingUsbPan = 0;
    pendingUsbButtons = lastSentUsbButtons;
    pendingUsbReport = false;
}

static void queueBleButtonSync(uint8_t buttons)
{
    pendingBleButtons = buttons;
    pendingBleReport = true;
}

static void queueUsbButtonSync(uint8_t buttons)
{
    pendingUsbButtons = buttons;
    pendingUsbReport = true;
}

static bool tryFlushPendingBleReport(bool forceRetry)
{
    if (!pendingBleReport || !bleMouseConnected()) return false;

    const uint32_t now = millis();
    const bool buttonChanged = pendingBleButtons != lastSentBleButtons;

    // A failed report is retried at most once per millisecond from the normal
    // update loop. New movement and button changes may always trigger an
    // immediate attempt.
    if (!forceRetry && !buttonChanged && now == lastBleRetryMs) {
        return false;
    }

    MouseReportData report;
    report.buttons = pendingBleButtons;
    report.x = peekClampedAxis(pendingBleX);
    report.y = peekClampedAxis(pendingBleY);
    report.wheel = peekClampedAxis(pendingBleWheel);
    report.pan = peekClampedAxis(pendingBlePan);

    lastBleRetryMs = now;

    if (!bleMouseTrySendReport(report)) {
        return false;
    }

    consumeAxis(pendingBleX, report.x);
    consumeAxis(pendingBleY, report.y);
    consumeAxis(pendingBleWheel, report.wheel);
    consumeAxis(pendingBlePan, report.pan);

    lastSentBleButtons = report.buttons;
    energyProfilerBleReport();
    usageStatsRecordMouseReport(false);

    pendingBleReport =
        pendingBleX != 0 || pendingBleY != 0 ||
        pendingBleWheel != 0 || pendingBlePan != 0 ||
        pendingBleButtons != lastSentBleButtons;

    return true;
}

static bool tryFlushPendingUsbReport(bool forceRetry)
{
    if (!pendingUsbReport || !usbMouseHostMounted()) return false;

    const uint32_t now = millis();
    const bool buttonChanged = pendingUsbButtons != lastSentUsbButtons;

    if (!forceRetry && !buttonChanged && now == lastUsbRetryMs) {
        return false;
    }

    MouseReportData report;
    report.buttons = pendingUsbButtons;
    report.x = peekClampedAxis(pendingUsbX);
    report.y = peekClampedAxis(pendingUsbY);
    report.wheel = peekClampedAxis(pendingUsbWheel);
    report.pan = peekClampedAxis(pendingUsbPan);

    lastUsbRetryMs = now;

    if (!usbMouseTrySendReport(report)) {
        return false;
    }

    consumeAxis(pendingUsbX, report.x);
    consumeAxis(pendingUsbY, report.y);
    consumeAxis(pendingUsbWheel, report.wheel);
    consumeAxis(pendingUsbPan, report.pan);

    lastSentUsbButtons = report.buttons;
    energyProfilerUsbReport();
    usageStatsRecordMouseReport(true);

    pendingUsbReport =
        pendingUsbX != 0 || pendingUsbY != 0 ||
        pendingUsbWheel != 0 || pendingUsbPan != 0 ||
        pendingUsbButtons != lastSentUsbButtons;

    return true;
}

static int8_t scaleAxis(int8_t value, float multiplier, float &remainder)
{
    if (value == 0) {
        return 0;
    }

    const float scaledValue =
        ((float)value * multiplier) + remainder;

    int output = (int)scaledValue;
    remainder = scaledValue - (float)output;

    if (output > 127) {
        output = 127;
        remainder = 0.0f;
    } else if (output < -127) {
        output = -127;
        remainder = 0.0f;
    }

    return (int8_t)output;
}

static void handleModeTransition()
{
    const bool usbMode = usbMouseHostMounted();

    if (usbMode == wasUsbMode) {
        return;
    }

    wasUsbMode = usbMode;
    usageStatsRecordOutputModeChange(usbMode);
    systemManagerRecordEvent(usbMode ? "USB HID active" : "Returned to BLE");
    resetScaleRemainders();

    const uint8_t currentButtons = buttonsCurrent();

#if DEBUG_ENABLED && DEBUG_USB
    Serial.print("Mouse output mode: ");
    Serial.println(usbMode ? "USB HID" : "BLE HID");
#endif

    if (usbMode) {
        // Movement intended for BLE must not leak into USB after the switch.
        // Immediately synchronize the current button state to the USB host.
        clearPendingBleReport();
        clearPendingUsbReport();
        queueUsbButtonSync(currentButtons);

        // BLE remains connected for a quick return, but receives no HID input.
        bleMouseSetInputEnabled(false);
        tryFlushPendingUsbReport(true);
        return;
    }

    // USB was unplugged. Drop unsent USB movement, restore BLE input and send
    // the current button state even when it equals our cached state. This
    // releases any button that the BLE host may still consider held.
    clearPendingUsbReport();
    clearPendingBleReport();
    queueBleButtonSync(currentButtons);

    bleMouseSetAdvertisingAllowed(true);
    bleMouseSetInputEnabled(true);

    if (!bleMouseConnected()) {
        bleMouseStartAdvertising();
    } else {
        tryFlushPendingBleReport(true);
    }
}

void mouseOutputBegin()
{
    usbMouseBegin();
    bleMouseBegin();

    wasUsbMode = usbMouseHostMounted();
    wasAirMode = airMouseIsActive();
    resetScaleRemainders();
    clearPendingBleReport();
    clearPendingUsbReport();

    bleMouseSetAdvertisingAllowed(true);
    bleMouseSetInputEnabled(!wasUsbMode);
}

void mouseOutputUpdate()
{
    usbMouseUpdate();
    handleModeTransition();

    // Retry reports that the active transport could not accept previously.
    if (usbMouseHostMounted()) {
        tryFlushPendingUsbReport(false);
    } else {
        tryFlushPendingBleReport(false);
    }
}

bool mouseOutputConnected()
{
    return usbMouseHostMounted() || bleMouseConnected();
}

bool mouseOutputCanSend()
{
    return usbMouseHostMounted() || bleMouseConnected();
}

void mouseOutputSendReport(const MouseReportData &report)
{
    const bool usbMode = usbMouseHostMounted();
    const bool airMode = airMouseIsActive();

    if (airMode != wasAirMode) {
        wasAirMode = airMode;
        resetScaleRemainders();
        clearPendingBleReport();
        clearPendingUsbReport();
    }

    const float multiplier =
        usbMode
            ? (airMode
                ? USB_AIR_MOUSE_SPEED_MULTIPLIER
                : USB_BALL_MOUSE_SPEED_MULTIPLIER)
            : (airMode
                ? BLE_AIR_MOUSE_SPEED_MULTIPLIER
                : BLE_BALL_MOUSE_SPEED_MULTIPLIER);

    MouseReportData outputReport = report;

    outputReport.x =
        scaleAxis(report.x, multiplier, xScaleRemainder);

    outputReport.y =
        scaleAxis(report.y, multiplier, yScaleRemainder);

    if (usbMode) {
        addSaturated(pendingUsbX, outputReport.x);
        addSaturated(pendingUsbY, outputReport.y);
        addSaturated(pendingUsbWheel, outputReport.wheel);
        addSaturated(pendingUsbPan, outputReport.pan);
        pendingUsbButtons = outputReport.buttons;
        pendingUsbReport = true;

        tryFlushPendingUsbReport(true);
        return;
    }

    addSaturated(pendingBleX, outputReport.x);
    addSaturated(pendingBleY, outputReport.y);
    addSaturated(pendingBleWheel, outputReport.wheel);
    addSaturated(pendingBlePan, outputReport.pan);
    pendingBleButtons = outputReport.buttons;
    pendingBleReport = true;

    // New movement and button changes are offered immediately. If Bluefruit's
    // notification queue is full, the values remain pending for retry.
    tryFlushPendingBleReport(true);
}
