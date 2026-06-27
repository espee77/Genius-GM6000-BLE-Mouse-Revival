#pragma once
#include <Arduino.h>
#include <LSM6DS3.h>

extern LSM6DS3 imu;

bool imuBegin();
void imuUpdate();
bool imuDetectPhysicalMovement();
float imuAccelX();
float imuAccelY();
float imuAccelZ();
float imuGyroX();
float imuGyroZ();
bool imuIsMouseOnBack();
bool imuIsAirMouseSide();
bool imuIsLeftSide();
bool imuIsRightSide();
