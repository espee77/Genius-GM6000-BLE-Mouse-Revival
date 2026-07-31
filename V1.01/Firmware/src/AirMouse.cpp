#include "AirMouse.h"
#include "Config.h"
#include "ImuManager.h"
#include <math.h>
#include <bluefruit.h>

static float gyroOffsetX = 0.0f;
static float gyroOffsetZ = 0.0f;
static float smoothGX = 0.0f;
static float smoothGZ = 0.0f;
static float accumX = 0.0f;
static float accumY = 0.0f;
static float lastAccelMagnitude = 0.0f;

// Click stabilization
static bool airLeftWasDown = false;
static bool airDragging = false;
static uint32_t airLeftDownTime = 0;
static int airClickMoveTotal = 0;

static float applyDeadzone(float value, float deadzone) {
    if (fabs(value) < deadzone) return 0.0f;
    return value;
}

static float applyAntiDeadzone(float value, float antiDeadzone) {
    if (value == 0.0f) return 0.0f;
    return value > 0.0f ? value + antiDeadzone : value - antiDeadzone;
}

static float limitSpike(float value, float limit) {
    if (value > limit) return limit;
    if (value < -limit) return -limit;
    return value;
}

static float applyAdaptiveAcceleration(
    float value,
    float baseSpeed,
    float zone1Multiplier,
    float zone2Multiplier,
    float zone3Multiplier
) {
    float sign = value >= 0.0f ? 1.0f : -1.0f;
    float absValue = fabs(value);

    float multiplier;
    if (absValue < AIR_MOUSE_ZONE_1_THRESHOLD) {
        multiplier = zone1Multiplier;
    } else if (absValue < AIR_MOUSE_ZONE_2_THRESHOLD) {
        multiplier = zone2Multiplier;
    } else {
        multiplier = zone3Multiplier;
    }

    return sign * absValue * baseSpeed * multiplier;
}

static int8_t gyroToMouseWithAccumulator(
    float gyroValue,
    float baseSpeed,
    float deadzone,
    float antiDeadzone,
    float zone1Multiplier,
    float zone2Multiplier,
    float zone3Multiplier,
    float stopThreshold,
    float &accumulator
) {
    float value = applyDeadzone(gyroValue, deadzone);

    if (fabs(value) < stopThreshold) {
        accumulator = 0.0f;
        return 0;
    }

    value = applyAntiDeadzone(value, antiDeadzone);

    float movementFloat = applyAdaptiveAcceleration(
        value,
        baseSpeed,
        zone1Multiplier,
        zone2Multiplier,
        zone3Multiplier
    );

    accumulator += movementFloat;
    int movement = (int)accumulator;
    accumulator -= movement;

    return constrain(movement, -127, 127);
}

static void calibrateGyro() {
    float sumX = 0.0f;
    float sumZ = 0.0f;

    for (int i = 0; i < AIR_MOUSE_CALIBRATION_SAMPLES; i++) {
        sumX += imu.readFloatGyroX();
        sumZ += imu.readFloatGyroZ();
        delay(5);
    }

    gyroOffsetX = sumX / AIR_MOUSE_CALIBRATION_SAMPLES;
    gyroOffsetZ = sumZ / AIR_MOUSE_CALIBRATION_SAMPLES;
}

static void autoRecalibrateGyro(
    float rawMeasuredGX,
    float rawMeasuredGZ,
    float correctedGX,
    float correctedGZ,
    float accelMagnitude
) {
#if AIR_MOUSE_AUTO_CALIBRATION_ENABLED
    float accelDelta = fabs(accelMagnitude - lastAccelMagnitude);

    bool gyroStill =
        fabs(correctedGX) < AIR_MOUSE_STILL_GYRO_THRESHOLD &&
        fabs(correctedGZ) < AIR_MOUSE_STILL_GYRO_THRESHOLD;

    bool accelStill = accelDelta < AIR_MOUSE_STILL_ACCEL_DELTA_THRESHOLD;

    if (gyroStill && accelStill) {
        gyroOffsetX =
            (1.0f - AIR_MOUSE_AUTO_CALIBRATION_RATE) * gyroOffsetX +
            AIR_MOUSE_AUTO_CALIBRATION_RATE * rawMeasuredGX;

        gyroOffsetZ =
            (1.0f - AIR_MOUSE_AUTO_CALIBRATION_RATE) * gyroOffsetZ +
            AIR_MOUSE_AUTO_CALIBRATION_RATE * rawMeasuredGZ;
    }
#endif

    lastAccelMagnitude = accelMagnitude;
}

static void applyClickStabilization(uint8_t buttons, int8_t &moveX, int8_t &moveY) {
    bool leftDown = buttons & MOUSE_BUTTON_LEFT;

    if (leftDown && !airLeftWasDown) {
        airLeftWasDown = true;
        airDragging = false;
        airLeftDownTime = millis();
        airClickMoveTotal = 0;

        moveX = 0;
        moveY = 0;
        accumX = 0.0f;
        accumY = 0.0f;
        return;
    }

    if (!leftDown && airLeftWasDown) {
        airLeftWasDown = false;
        airDragging = false;
        airClickMoveTotal = 0;

        moveX = 0;
        moveY = 0;
        accumX = 0.0f;
        accumY = 0.0f;
        return;
    }

    if (!leftDown) return;

    uint32_t heldTime = millis() - airLeftDownTime;

    airClickMoveTotal += abs((int)moveX) + abs((int)moveY);

    if (!airDragging &&
        heldTime > AIR_MOUSE_CLICK_STABILIZE_MS &&
        airClickMoveTotal >= AIR_MOUSE_DRAG_START_PIXELS) {
        airDragging = true;
    }

    if (!airDragging) {
        moveX = 0;
        moveY = 0;
        accumX = 0.0f;
        accumY = 0.0f;
    }
}

void airMouseBegin() {
#if AIR_MOUSE_ENABLED
    calibrateGyro();

    float ax = imuAccelX();
    float ay = imuAccelY();
    float az = imuAccelZ();

    lastAccelMagnitude = sqrt(ax * ax + ay * ay + az * az);
#endif
}

bool airMouseIsActive() {
#if AIR_MOUSE_ENABLED
    return imuIsAirMouseSide();
#else
    return false;
#endif
}

void airMouseReset() {
    smoothGX = 0.0f;
    smoothGZ = 0.0f;
    accumX = 0.0f;
    accumY = 0.0f;

    airLeftWasDown = false;
    airDragging = false;
    airLeftDownTime = 0;
    airClickMoveTotal = 0;
}

MouseReportData airMouseUpdate(uint8_t buttons, bool &activity) {
    MouseReportData report;
    report.buttons = buttons;

#if AIR_MOUSE_ENABLED
    float ax = imuAccelX();
    float ay = imuAccelY();
    float az = imuAccelZ();

    float rawMeasuredGX = imuGyroX();
    float rawMeasuredGZ = imuGyroZ();

    float rawGX = rawMeasuredGX - gyroOffsetX;
    float rawGZ = rawMeasuredGZ - gyroOffsetZ;

    float accelMagnitude = sqrt(ax * ax + ay * ay + az * az);

    autoRecalibrateGyro(rawMeasuredGX, rawMeasuredGZ, rawGX, rawGZ, accelMagnitude);

    bool leftSide = imuIsLeftSide();
    bool rightSide = imuIsRightSide();

    if (leftSide || rightSide) {
        smoothGX =
            AIR_MOUSE_SMOOTHING_X * smoothGX +
            (1.0f - AIR_MOUSE_SMOOTHING_X) * rawGX;

        smoothGZ =
            AIR_MOUSE_SMOOTHING_Y * smoothGZ +
            (1.0f - AIR_MOUSE_SMOOTHING_Y) * rawGZ;

#if AIR_MOUSE_TREMOR_FILTER_ENABLED
        if (fabs(smoothGX) < AIR_MOUSE_TREMOR_THRESHOLD_X) {
            smoothGX *= AIR_MOUSE_TREMOR_REDUCTION;
        }

        if (fabs(smoothGZ) < AIR_MOUSE_TREMOR_THRESHOLD_Y) {
            smoothGZ *= AIR_MOUSE_TREMOR_REDUCTION;
        }
#endif

        smoothGX = limitSpike(smoothGX, AIR_MOUSE_SPIKE_LIMIT_X);
        smoothGZ = limitSpike(smoothGZ, AIR_MOUSE_SPIKE_LIMIT_Y);

        int8_t moveX = gyroToMouseWithAccumulator(
            smoothGX,
            AIR_MOUSE_SPEED_X,
            AIR_MOUSE_DEADZONE_X,
            AIR_MOUSE_ANTI_DEADZONE_X,
            AIR_MOUSE_ZONE_1_MULTIPLIER_X,
            AIR_MOUSE_ZONE_2_MULTIPLIER_X,
            AIR_MOUSE_ZONE_3_MULTIPLIER_X,
            AIR_MOUSE_STOP_DAMPING_THRESHOLD_X,
            accumX
        );

        int8_t moveY = gyroToMouseWithAccumulator(
            smoothGZ,
            AIR_MOUSE_SPEED_Y,
            AIR_MOUSE_DEADZONE_Y,
            AIR_MOUSE_ANTI_DEADZONE_Y,
            AIR_MOUSE_ZONE_1_MULTIPLIER_Y,
            AIR_MOUSE_ZONE_2_MULTIPLIER_Y,
            AIR_MOUSE_ZONE_3_MULTIPLIER_Y,
            AIR_MOUSE_STOP_DAMPING_THRESHOLD_Y,
            accumY
        );

        if (INVERT_AIR_X) moveX = -moveX;
        if (INVERT_AIR_Y) moveY = -moveY;

        // GM-6000 orientation: left side needs extra vertical inversion.
        if (leftSide) moveY = -moveY;

        applyClickStabilization(buttons, moveX, moveY);

        report.x = moveX;
        report.y = moveY;
    } else {
        airMouseReset();
    }
#endif

    activity = report.hasMotion();
    return report;
}