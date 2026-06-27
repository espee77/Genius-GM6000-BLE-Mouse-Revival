#pragma once

// =====================================================
// DEBUG CONFIGURATION
// =====================================================

#define DEBUG_ENABLED       false
#define DEBUG_BLE           false
#define DEBUG_BUTTONS       false
#define DEBUG_MULTI_HOST    false
#define DEBUG_MOVEMENT      false
#define DEBUG_SLEEP         false
#define DEBUG_BATTERY       false
#define DEBUG_WAKE          false
#define DEBUG_IMU           false
#define DEBUG_LED           false

// =====================================================
// BALL MOUSE CONFIGURATION
// =====================================================

#define MOUSE_SPEED 3

// Cursor scroll speed multiplier
#define SCROLL_SPEED 1

// 0 = very slow, 5 = medium, 10 = very fast
#define SCROLL_SENSITIVITY 4

// Smooth scroll tuning
#define SCROLL_MOMENTUM_DAMPING 0.82f
#define SCROLL_EXIT_DAMPING 0.70f
#define SCROLL_STOP_VELOCITY_THRESHOLD 0.03f
#define SCROLL_STOP_ACCUMULATOR_THRESHOLD 0.05f
#define SCROLL_MAX_DELTA 127

#define INVERT_X false
#define INVERT_Y false
#define INVERT_SCROLL true

#define BUTTON_DEBOUNCE_MS 3

// =====================================================
// POWER CONFIGURATION
// =====================================================

#define SLEEP_TIMEOUT_MS       5000UL      // 5 seconden
#define DEEP_SLEEP_TIMEOUT_MS 1800000UL     // 30 minuten
#define SLEEP_COUNTDOWN_PRINT_INTERVAL_MS 15000
#define POST_WAKE_ENCODER_SETTLE_MS 20

// =====================================================
// BLE CONFIGURATION
// =====================================================

#define BLE_DEVICE_NAME "Genius GM-6000 BLE mouse"
#define BLE_TX_POWER 8

// =====================================================
// BATTERY CONFIGURATION
// =====================================================

#define BATTERY_UPDATE_INTERVAL_MS 10000
#define BATTERY_DIVIDER_FACTOR 3.03f

// =====================================================
// IMU CONFIGURATION
// =====================================================

#define IMU_WAKE_THRESHOLD 0.03f
#define IMU_POLL_INTERVAL_MS 100

// =====================================================
// LED CONFIGURATION
// =====================================================

#define STATUS_LED_UPDATE_INTERVAL_MS 100

#define BLUE_ADVERTISING_PULSE_INTERVAL_MS 3000
#define BLUE_ADVERTISING_PULSE_ON_MS 80

#define RED_BATTERY_STATUS_INTERVAL_MS 3000
#define RED_BATTERY_BLINK_ON_MS 120
#define RED_BATTERY_BLINK_GAP_MS 180

#define EXTERNAL_LED_ACTIVE_LOW false

#define MOUSE_ON_BACK_Z_THRESHOLD -0.65f

// =====================================================
// MAIN LOOP
// =====================================================

#define MAIN_LOOP_DELAY_MS 2

// =====================================================
// AIR MOUSE CONFIGURATION - v6c stable basis
// =====================================================

#define AIR_MOUSE_ENABLED true

#define AIR_MOUSE_SIDE_THRESHOLD 0.75f

#define AIR_MOUSE_SPEED_X 0.55f
#define AIR_MOUSE_SPEED_Y 0.55f

#define AIR_MOUSE_DEADZONE_X 0.25f
#define AIR_MOUSE_DEADZONE_Y 0.85f

#define AIR_MOUSE_ANTI_DEADZONE_X 0.40f
#define AIR_MOUSE_ANTI_DEADZONE_Y 0.35f

#define AIR_MOUSE_SMOOTHING_X 0.15f
#define AIR_MOUSE_SMOOTHING_Y 0.30f

#define AIR_MOUSE_TREMOR_FILTER_ENABLED true

#define AIR_MOUSE_TREMOR_THRESHOLD_X 0.65f
#define AIR_MOUSE_TREMOR_THRESHOLD_Y 0.75f

#define AIR_MOUSE_TREMOR_REDUCTION 0.45f

#define AIR_MOUSE_SPIKE_LIMIT_X 600.0f
#define AIR_MOUSE_SPIKE_LIMIT_Y 600.0f

#define INVERT_AIR_X true
#define INVERT_AIR_Y true

#define AIR_MOUSE_CALIBRATION_SAMPLES 200

#define AIR_MOUSE_AUTO_CALIBRATION_ENABLED true
#define AIR_MOUSE_STILL_GYRO_THRESHOLD 0.8f
#define AIR_MOUSE_STILL_ACCEL_DELTA_THRESHOLD 0.025f
#define AIR_MOUSE_AUTO_CALIBRATION_RATE 0.002f

#define AIR_MOUSE_STOP_DAMPING_THRESHOLD_X 0.15f
#define AIR_MOUSE_STOP_DAMPING_THRESHOLD_Y 0.25f

#define AIR_MOUSE_ZONE_1_THRESHOLD 4.0f
#define AIR_MOUSE_ZONE_2_THRESHOLD 12.0f

#define AIR_MOUSE_ZONE_1_MULTIPLIER_X 1.0f
#define AIR_MOUSE_ZONE_2_MULTIPLIER_X 1.6f
#define AIR_MOUSE_ZONE_3_MULTIPLIER_X 2.4f

#define AIR_MOUSE_ZONE_1_MULTIPLIER_Y 1.0f
#define AIR_MOUSE_ZONE_2_MULTIPLIER_Y 1.35f
#define AIR_MOUSE_ZONE_3_MULTIPLIER_Y 2.0f

#define AIR_MOUSE_CLICK_STABILIZE_MS 650
#define AIR_MOUSE_DRAG_START_PIXELS 80

#define AIR_MOUSE_BUTTON_MOVE_SCALE 0.0f