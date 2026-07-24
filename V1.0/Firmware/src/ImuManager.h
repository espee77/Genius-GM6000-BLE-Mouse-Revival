#pragma once
#include <Arduino.h>
#include <LSM6DS3.h>

extern LSM6DS3 imu;

bool imuBegin();

// Power profiles
void imuSetBallProfile();
void imuSetAirProfile();
void imuSetIdleProfile();

// Cached sensor updates
void imuUpdate();
void imuUpdateAccelOnly();

bool imuDetectPhysicalMovement();
void imuResetWakeBaseline();

float imuAccelX();
float imuAccelY();
float imuAccelZ();
float imuGyroX();
float imuGyroZ();

bool imuIsMouseOnBack();
bool imuIsAirMouseSide();
bool imuIsLeftSide();
bool imuIsRightSide();
