#include "ImuManager.h"
#include "Config.h"
#include <Wire.h>
#include <math.h>

LSM6DS3 imu(I2C_MODE, 0x6A);

// LSM6DS3 control registers.
static const uint8_t REG_CTRL1_XL = 0x10;
static const uint8_t REG_CTRL2_G  = 0x11;

// ODR values in the upper nibble of CTRL1_XL / CTRL2_G.
static const uint8_t ODR_POWER_DOWN = 0x00;
static const uint8_t ODR_12_5_HZ    = 0x10;
static const uint8_t ODR_26_HZ      = 0x20;
static const uint8_t ODR_104_HZ     = 0x40;

enum ImuProfile {
    IMU_PROFILE_UNKNOWN,
    IMU_PROFILE_BALL,
    IMU_PROFILE_AIR,
    IMU_PROFILE_IDLE
};

static ImuProfile currentProfile = IMU_PROFILE_UNKNOWN;

static float ax = 0.0f;
static float ay = 0.0f;
static float az = 0.0f;
static float gx = 0.0f;
static float gz = 0.0f;
static float lastAccelMagnitude = 0.0f;

static float wakeBaseAx = 0.0f;
static float wakeBaseAy = 0.0f;
static float wakeBaseAz = 0.0f;
static bool wakeBaselineValid = false;
static uint8_t wakeMovementHits = 0;
static uint32_t wakeFirstHitTime = 0;

static void setRegisterOdr(uint8_t reg, uint8_t odr)
{
    uint8_t value = 0;
    if (imu.readRegister(&value, reg) != IMU_SUCCESS) {
        return;
    }

    value = (value & 0x0F) | odr;
    imu.writeRegister(reg, value);
}

bool imuBegin()
{
    if (imu.begin() != 0) {
        return false;
    }

    // Startup calibration in AirMouse.cpp needs valid gyro data.
    setRegisterOdr(REG_CTRL1_XL, ODR_104_HZ);
    setRegisterOdr(REG_CTRL2_G, ODR_104_HZ);
    currentProfile = IMU_PROFILE_AIR;
    delay(IMU_GYRO_STARTUP_MS);

    imuUpdate();
    lastAccelMagnitude = sqrt(ax * ax + ay * ay + az * az);
    return true;
}

void imuSetBallProfile()
{
    if (currentProfile == IMU_PROFILE_BALL) return;

    // 26 Hz accelerometer is sufficient to detect side orientation.
    // Gyroscope is fully powered down.
    setRegisterOdr(REG_CTRL1_XL, ODR_26_HZ);
    setRegisterOdr(REG_CTRL2_G, ODR_POWER_DOWN);
    currentProfile = IMU_PROFILE_BALL;

#if DEBUG_ENABLED && DEBUG_IMU
    Serial.println("IMU profile: BALL (accel 26 Hz, gyro off)");
#endif
}

void imuSetAirProfile()
{
    if (currentProfile == IMU_PROFILE_AIR) return;

    setRegisterOdr(REG_CTRL1_XL, ODR_104_HZ);
    setRegisterOdr(REG_CTRL2_G, ODR_104_HZ);
    currentProfile = IMU_PROFILE_AIR;

    // Allow the gyro output to become valid after power-up.
    delay(IMU_GYRO_STARTUP_MS);
    imuUpdate();

#if DEBUG_ENABLED && DEBUG_IMU
    Serial.println("IMU profile: AIR (accel + gyro 104 Hz)");
#endif
}

void imuSetIdleProfile()
{
    if (currentProfile == IMU_PROFILE_IDLE) return;

    // 26 Hz keeps the existing 3-hits-within-150-ms wake detector reliable.
    // Gyroscope remains fully powered down.
    setRegisterOdr(REG_CTRL1_XL, ODR_26_HZ);
    setRegisterOdr(REG_CTRL2_G, ODR_POWER_DOWN);
    currentProfile = IMU_PROFILE_IDLE;

#if DEBUG_ENABLED && DEBUG_IMU
    Serial.println("IMU profile: IDLE (accel 26 Hz, gyro off)");
#endif
}

void imuUpdateAccelOnly()
{
    ax = imu.readFloatAccelX();
    ay = imu.readFloatAccelY();
    az = imu.readFloatAccelZ();
}

void imuUpdate()
{
    imuUpdateAccelOnly();
    gx = imu.readFloatGyroX();
    gz = imu.readFloatGyroZ();
}

void imuResetWakeBaseline()
{
    imuUpdateAccelOnly();

    wakeBaseAx = ax;
    wakeBaseAy = ay;
    wakeBaseAz = az;
    wakeBaselineValid = true;

    wakeMovementHits = 0;
    wakeFirstHitTime = 0;

#if DEBUG_ENABLED && DEBUG_IMU
    Serial.println("IMU wake baseline reset");
#endif
}

bool imuDetectPhysicalMovement()
{
    imuUpdateAccelOnly();

    if (!wakeBaselineValid) {
        imuResetWakeBaseline();
        return false;
    }

    const float dx = fabs(ax - wakeBaseAx);
    const float dy = fabs(ay - wakeBaseAy);
    const float dz = fabs(az - wakeBaseAz);

    wakeBaseAx = ax;
    wakeBaseAy = ay;
    wakeBaseAz = az;

    const float movement = dx + dy + dz;
    const uint32_t now = millis();

    if (movement > IMU_WAKE_THRESHOLD) {
        if (wakeMovementHits == 0) {
            wakeMovementHits = 1;
            wakeFirstHitTime = now;
        } else if (now - wakeFirstHitTime <= IMU_WAKE_HIT_WINDOW_MS) {
            wakeMovementHits++;
        } else {
            wakeMovementHits = 1;
            wakeFirstHitTime = now;
        }
    } else {
        wakeMovementHits = 0;
        wakeFirstHitTime = 0;
    }

#if DEBUG_ENABLED && DEBUG_IMU
    Serial.print("IMU wake movement: ");
    Serial.print(movement, 4);
    Serial.print(" hits: ");
    Serial.println(wakeMovementHits);
#endif

    if (wakeMovementHits >= IMU_WAKE_REQUIRED_HITS) {
        wakeMovementHits = 0;
        wakeFirstHitTime = 0;
        return true;
    }

    return false;
}

float imuAccelX() { return ax; }
float imuAccelY() { return ay; }
float imuAccelZ() { return az; }
float imuGyroX() { return gx; }
float imuGyroZ() { return gz; }

bool imuIsMouseOnBack() {
    return az < MOUSE_ON_BACK_Z_THRESHOLD;
}

bool imuIsLeftSide() {
    return ay > AIR_MOUSE_SIDE_THRESHOLD;
}

bool imuIsRightSide() {
    return ay < -AIR_MOUSE_SIDE_THRESHOLD;
}

bool imuIsAirMouseSide() {
    return imuIsLeftSide() || imuIsRightSide();
}
