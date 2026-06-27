#include "ImuManager.h"
#include "Config.h"
#include <Wire.h>
#include <math.h>

LSM6DS3 imu(I2C_MODE, 0x6A);

static float ax = 0.0f;
static float ay = 0.0f;
static float az = 0.0f;
static float gx = 0.0f;
static float gz = 0.0f;
static float lastAccelMagnitude = 0.0f;

bool imuBegin() {
    if (imu.begin() != 0) {
        return false;
    }

    imuUpdate();
    lastAccelMagnitude = sqrt(ax * ax + ay * ay + az * az);
    return true;
}

void imuUpdate() {
    ax = imu.readFloatAccelX();
    ay = imu.readFloatAccelY();
    az = imu.readFloatAccelZ();
    gx = imu.readFloatGyroX();
    gz = imu.readFloatGyroZ();
}

bool imuDetectPhysicalMovement() {
    static float lastAx = 0.0f;
    static float lastAy = 0.0f;
    static float lastAz = 0.0f;
    static bool initialized = false;

    imuUpdate();

    if (!initialized) {
        lastAx = ax;
        lastAy = ay;
        lastAz = az;
        initialized = true;
        return false;
    }

    float dx = fabs(ax - lastAx);
    float dy = fabs(ay - lastAy);
    float dz = fabs(az - lastAz);

    lastAx = ax;
    lastAy = ay;
    lastAz = az;

    float movement = dx + dy + dz;

    return movement > IMU_WAKE_THRESHOLD;
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
