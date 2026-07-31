#pragma once

// ============================================================================
// GM-6000 FIRMWARE CONFIGURATION
// ============================================================================
//
// Central configuration file for:
// - Debug output
// - USB and BLE HID
// - Ball mouse movement and scrolling
// - IR LED power saving
// - Sleep and wake behaviour
// - Battery monitoring
// - Status LEDs
// - Air mouse behaviour
//
// Change settings here instead of editing the implementation files.
// ============================================================================


// ============================================================================
// FIRMWARE VERSION
// ============================================================================
//
// Exposed through the BLE Device Information Service.
// Increase PATCH for bug fixes, MINOR for new compatible features,
// and MAJOR for incompatible changes.
//

#define FIRMWARE_VERSION_STRING "1.0.0"
#define FIRMWARE_HARDWARE_REV   "XIAO nRF52840 Sense"

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================
//
// DEBUG_ENABLED must be true before any individual debug category can print.
//
// Example:
//   DEBUG_ENABLED = true
//   DEBUG_USB     = true
//
// Set everything to false for normal release builds.
//

#define DEBUG_ENABLED       false

#define DEBUG_BLE           false
#define DEBUG_USB           false
#define DEBUG_SLEEP         false
#define DEBUG_BATTERY       false
#define DEBUG_IMU           false

// Optional runtime energy/work profiler. Enable temporarily while measuring
// through the Serial Monitor at 115200 baud, then set false for normal use.
#define ENERGY_PROFILER_ENABLED     false

// Lightweight long-term usage statistics with USB Serial commands.
// Statistics are saved to internal flash once per hour and at safe events.
#define USAGE_STATS_ENABLED          true



// ============================================================================
// SYSTEM RELIABILITY
// ============================================================================

#define SYSTEM_WATCHDOG_ENABLED              true
#define SYSTEM_WATCHDOG_TIMEOUT_MS            8000UL
#define BLE_SELF_HEAL_INTERVAL_MS             3000UL
#define CRITICAL_BATTERY_CHECK_INTERVAL_MS   10000UL
#define CRITICAL_BATTERY_SHUTDOWN_V             3.40f
#define CRITICAL_BATTERY_RECOVERY_V             3.50f
#define CRITICAL_BATTERY_CONFIRM_SAMPLES           3

// ============================================================================
// USB DEVICE CONFIGURATION
// ============================================================================
//
// Product and manufacturer names shown by the USB host.
//
// Windows may cache an old USB name. Remove the old device in Device Manager
// when changing these strings.
//

#define USB_PRODUCT_NAME      "Genius GM-6000 USB Mouse"
#define USB_MANUFACTURER_NAME "GM-6000 Revival Project"


// ============================================================================
// BLE CONFIGURATION
// ============================================================================
//
// BLE_DEVICE_NAME:
// Name shown during Bluetooth pairing.
//
// BLE_TX_POWER:
// Nordic nRF52840 transmit power in dBm.
// Common values: -40, -20, -16, -12, -8, -4, 0, 4, 8
// Higher values improve signal strength but use slightly more power.
//

#define BLE_DEVICE_NAME "Genius GM-6000 BLE mouse"
#define BLE_TX_POWER    8

// BLE connection interval units are 1.25 ms.
// Supervision-timeout units are 10 ms.
//
// Active profile:
//   interval 6   = 7.5 ms
//   timeout 200  = 2.0 s
//
// The experimental idle profile remains reserved but is not requested during
// normal operation. Earlier testing with a 500 ms interval caused Windows 11
// to return too slowly to the active profile, resulting in choppy movement.
#define BLE_ACTIVE_CONN_INTERVAL 6
#define BLE_ACTIVE_SUP_TIMEOUT   200
#define BLE_IDLE_CONN_INTERVAL   400   // 500 ms; reserved
#define BLE_IDLE_SUP_TIMEOUT     1200  // 12 s; reserved


// ============================================================================
// MOUSE OUTPUT SENSITIVITY
// ============================================================================
//
// Final movement scaling applied separately to BLE and USB reports.
//
// Examples:
//   1.00f = unchanged
//   1.10f = 10% faster
//   1.25f = 25% faster
//   0.90f = 10% slower
//
// These multipliers are applied after the normal ball-mouse calculations.
//

// Ball mouse
#define BLE_BALL_MOUSE_SPEED_MULTIPLIER 1.00f
#define USB_BALL_MOUSE_SPEED_MULTIPLIER 1.50f

// Air mouse
#define BLE_AIR_MOUSE_SPEED_MULTIPLIER 1.00f
#define USB_AIR_MOUSE_SPEED_MULTIPLIER 0.30f


// ============================================================================
// IR LED PWM CONFIGURATION
// ============================================================================
//
// The IR LEDs illuminate the original encoder wheels.
//
// IR_LED_PWM_ENABLED:
//   true  = use PWM to reduce average IR LED current
//   false = IR LEDs run continuously at full power
//
// IR_PWM_DUTY_PERCENT:
// Percentage of time the LEDs are powered.
// Examples:
//   100 = full power
//    50 = half average power
//    40 = current stable and lowest reliable setting
//
// Values below 40% caused unreliable encoder detection in testing.
//
// IR_PWM_RESOLUTION_BITS:
// PWM resolution.
//   8 bits = values from 0 to 255
//  10 bits = values from 0 to 1023
//
// 8 bits is more than sufficient for the IR LEDs.
//

#define IR_LED_PWM_ENABLED     true
#define IR_PWM_DUTY_PERCENT    40
#define IR_PWM_RESOLUTION_BITS 8


// ============================================================================
// BALL MOUSE MOVEMENT CONFIGURATION
// ============================================================================
//
// MOUSE_SPEED:
// Base multiplier for ball-mouse movement.
// Examples:
//   1 = slow
//   2 = medium
//   3 = current preferred setting
//   4 = fast
//
// INVERT_X / INVERT_Y:
// Reverse cursor direction for an axis.
//

#define MOUSE_SPEED 3

#define INVERT_X false
#define INVERT_Y false


// ============================================================================
// MIDDLE-BUTTON SCROLL CONFIGURATION
// ============================================================================
//
// SCROLL_SPEED:
// Whole-number multiplier applied to generated scroll steps.
// Examples:
//   1 = normal
//   2 = twice as fast
//
// SCROLL_SENSITIVITY:
// Controls how much ball movement is needed to generate scrolling.
// Suggested range:
//    0 = very slow
//    5 = medium
//   10 = very fast
//
// INVERT_SCROLL:
// true reverses the scroll direction.
//
// Momentum settings:
// Higher damping values retain movement longer.
// Lower values stop scrolling faster.
//

#define SCROLL_SPEED       1
#define SCROLL_SENSITIVITY 2

#define INVERT_SCROLL true

#define SCROLL_MOMENTUM_DAMPING            0.82f
#define SCROLL_EXIT_DAMPING                0.70f
#define SCROLL_STOP_VELOCITY_THRESHOLD     0.03f
#define SCROLL_STOP_ACCUMULATOR_THRESHOLD  0.05f


// ============================================================================
// BUTTON CONFIGURATION
// ============================================================================
//
// BUTTON_DEBOUNCE_MS:
// Left, right and middle presses are accepted immediately for low latency.
// Their release must remain stable for this duration before it is accepted.
// The bottom/multi-host button uses this value for conventional debounce.
//
// Suggested values:
//   2-3 ms  = fast response with healthy switches
//   5-10 ms = stronger filtering for noisy or worn switches
//

#define BUTTON_DEBOUNCE_MS 2


// ============================================================================
// POWER AND SLEEP CONFIGURATION
// ============================================================================
//
// SLEEP_TIMEOUT_MS:
// Time without activity before entering idle sleep.
//
// Examples:
//      3000UL = 3 seconds
//     30000UL = 30 seconds
//    120000UL = 2 minutes
//
// DEEP_SLEEP_TIMEOUT_MS:
// Time without activity before entering deep sleep.
//
// Examples:
//    600000UL = 10 minutes
//   1800000UL = 30 minutes
//   3600000UL = 60 minutes
//
// POST_WAKE_ENCODER_SETTLE_MS:
// Short delay after wake-up to allow the IR LEDs and encoder signals to settle.
//
// Current values:
//   idle sleep after 3 seconds
//   deep sleep after 10 minutes
//

#define SLEEP_TIMEOUT_MS       3000UL
#define DEEP_SLEEP_TIMEOUT_MS  600000UL

#define POST_WAKE_ENCODER_SETTLE_MS       20UL


// ============================================================================
// BATTERY CONFIGURATION
// ============================================================================
//
// BATTERY_UPDATE_INTERVAL_MS:
// Time between battery measurements.
//
// Examples:
//     10000UL = 10 seconds
//     60000UL = 1 minute
//    300000UL = 5 minutes
//
// BATTERY_DIVIDER_FACTOR:
// Calibration factor used to convert the ADC reading into battery voltage.
// Only change this after comparing the reported voltage with a multimeter.
//

#define BATTERY_UPDATE_INTERVAL_MS 10000UL
#define BATTERY_DIVIDER_FACTOR     3.03f

// Charge status is read directly from the on-board BQ25101 ~CHG output.
// The signal is debounced because it can briefly be inactive after USB power-up.
#define CHARGE_STATUS_POLL_INTERVAL_MS 100UL
#define CHARGE_STATUS_DEBOUNCE_MS      1000UL


// ============================================================================
// IMU WAKE CONFIGURATION
// ============================================================================
//
// The IMU is used to detect physical movement while the mouse is idle.
//
// IMU_WAKE_THRESHOLD:
// Minimum accelerometer change counted as movement.
// Lower value  = more sensitive
// Higher value = less sensitive
//
// IMU_POLL_INTERVAL_MS:
// Time between IMU checks during idle sleep.
//
// IMU_WAKE_REQUIRED_HITS:
// Number of valid movement detections required before waking.
//
// IMU_WAKE_HIT_WINDOW_MS:
// Maximum time in which all required hits must occur.
//
// Example current behaviour:
// - Check every 50 ms
// - Detect movement above 0.02
// - Require 3 hits within 150 ms
//

#define IMU_WAKE_THRESHOLD       0.02f
#define IMU_POLL_INTERVAL_MS     50UL
#define IMU_WAKE_REQUIRED_HITS   3
#define IMU_WAKE_HIT_WINDOW_MS   150UL

// IMU power profiles.
// Ball mode only needs the accelerometer for orientation detection.
// Air mode needs both accelerometer and gyroscope.
// Idle uses the slowest practical accelerometer rate and keeps gyro off.
#define IMU_BALL_POLL_INTERVAL_MS 25UL
#define IMU_AIR_POLL_INTERVAL_MS  5UL
#define IMU_GYRO_STARTUP_MS        20UL
#define IDLE_LOOP_DELAY_MS         20UL


// ============================================================================
// STATUS LED CONFIGURATION
// ============================================================================
//
// STATUS_LED_UPDATE_INTERVAL_MS:
// Refresh interval for external LED behaviour.
//
// Blue LED:
// Pulses while BLE is advertising.
//
// Red LED:
// Used for battery-status blink patterns.
//
// EXTERNAL_LED_ACTIVE_LOW:
// false = HIGH switches LED on
// true  = LOW switches LED on
//
// MOUSE_ON_BACK_Z_THRESHOLD:
// Accelerometer Z threshold used to determine whether the mouse is upside down.
// The external status LEDs are only shown in the intended orientation.
//

#define STATUS_LED_UPDATE_INTERVAL_MS 100UL

#define BLUE_ADVERTISING_PULSE_INTERVAL_MS 3000UL
#define BLUE_ADVERTISING_PULSE_ON_MS       80UL

#define RED_BATTERY_STATUS_INTERVAL_MS 3000UL
#define RED_BATTERY_BLINK_ON_MS        120UL
#define RED_BATTERY_BLINK_GAP_MS       180UL

#define EXTERNAL_LED_ACTIVE_LOW false

#define MOUSE_ON_BACK_Z_THRESHOLD -0.65f


// ============================================================================
// MAIN LOOP CONFIGURATION
// ============================================================================
//
// MAIN_LOOP_DELAY_MS:
// Small delay at the end of the main loop.
//
// Examples:
//   0-1 ms = maximum update rate
//   2 ms   = current stable setting
//   5 ms   = lower CPU activity but slower response
//

#define MAIN_LOOP_DELAY_MS          2UL
#define BALL_ACTIVE_LOOP_DELAY_MS   2UL
#define BALL_QUIET_LOOP_DELAY_MS    5UL


// ============================================================================
// AIR MOUSE GENERAL CONFIGURATION
// ============================================================================
//
// AIR_MOUSE_ENABLED:
// Enables or disables the complete air-mouse feature.
//
// AIR_MOUSE_SIDE_THRESHOLD:
// Determines how far the mouse must be tilted onto its side before air-mouse
// mode becomes active.
//
// Lower value  = activates more easily
// Higher value = requires a stronger side orientation
//

#define AIR_MOUSE_ENABLED        true
#define AIR_MOUSE_SIDE_THRESHOLD 0.75f


// ============================================================================
// AIR MOUSE SPEED AND DIRECTION
// ============================================================================
//
// AIR_MOUSE_SPEED_X / Y:
// Base gyro-to-cursor sensitivity.
//
// Examples:
//   0.30f = slow
//   0.55f = current stable setting
//   0.80f = fast
//
// INVERT_AIR_X / Y:
// Reverse movement direction for the corresponding axis.
//

#define AIR_MOUSE_SPEED_X 0.55f
#define AIR_MOUSE_SPEED_Y 0.55f

#define INVERT_AIR_X true
#define INVERT_AIR_Y true


// ============================================================================
// AIR MOUSE DEADZONE CONFIGURATION
// ============================================================================
//
// DEADZONE:
// Ignores very small gyro movement to reduce drift and hand tremor.
// Higher value = steadier cursor, but more movement is needed to start.
//
// ANTI_DEADZONE:
// Adds a minimum output once movement passes the deadzone.
// Higher value = cursor starts more decisively.
//

#define AIR_MOUSE_DEADZONE_X 0.25f
#define AIR_MOUSE_DEADZONE_Y 0.85f

#define AIR_MOUSE_ANTI_DEADZONE_X 0.40f
#define AIR_MOUSE_ANTI_DEADZONE_Y 0.35f


// ============================================================================
// AIR MOUSE SMOOTHING CONFIGURATION
// ============================================================================
//
// Smoothing reduces abrupt movement changes.
//
// Lower value  = smoother but slower response
// Higher value = more direct but potentially shakier
//

#define AIR_MOUSE_SMOOTHING_X 0.15f
#define AIR_MOUSE_SMOOTHING_Y 0.30f


// ============================================================================
// AIR MOUSE TREMOR FILTER
// ============================================================================
//
// TREMOR_FILTER_ENABLED:
// Enables extra reduction of small involuntary movements.
//
// TREMOR_THRESHOLD:
// Movement below this level is treated as possible tremor.
//
// TREMOR_REDUCTION:
// Scale applied to movement identified as tremor.
//
// Examples:
//   0.25f = strong reduction
//   0.45f = current setting
//   0.75f = light reduction
//

#define AIR_MOUSE_TREMOR_FILTER_ENABLED true

#define AIR_MOUSE_TREMOR_THRESHOLD_X 0.65f
#define AIR_MOUSE_TREMOR_THRESHOLD_Y 0.75f

#define AIR_MOUSE_TREMOR_REDUCTION 0.45f


// ============================================================================
// AIR MOUSE SPIKE LIMITER
// ============================================================================
//
// Limits sudden extreme gyro readings.
//
// Lower values block large jumps more aggressively.
// Very low values may limit legitimate fast movement.
//

#define AIR_MOUSE_SPIKE_LIMIT_X 600.0f
#define AIR_MOUSE_SPIKE_LIMIT_Y 600.0f


// ============================================================================
// AIR MOUSE CALIBRATION
// ============================================================================
//
// AIR_MOUSE_CALIBRATION_SAMPLES:
// Number of samples used during startup calibration.
// More samples improve averaging but increase startup time.
//
// AUTO_CALIBRATION:
// Slowly corrects gyro offset while the mouse is detected as stationary.
//
// STILL_GYRO_THRESHOLD:
// Maximum gyro movement considered stationary.
//
// STILL_ACCEL_DELTA_THRESHOLD:
// Maximum accelerometer variation considered stationary.
//
// AUTO_CALIBRATION_RATE:
// Rate at which the saved gyro offset is corrected.
// Examples:
//   0.001f = slow correction
//   0.002f = current setting
//   0.005f = faster correction
//

#define AIR_MOUSE_CALIBRATION_SAMPLES 200

#define AIR_MOUSE_AUTO_CALIBRATION_ENABLED true
#define AIR_MOUSE_STILL_GYRO_THRESHOLD     0.8f
#define AIR_MOUSE_STILL_ACCEL_DELTA_THRESHOLD 0.025f
#define AIR_MOUSE_AUTO_CALIBRATION_RATE    0.002f


// ============================================================================
// AIR MOUSE STOP DAMPING
// ============================================================================
//
// Helps the cursor settle when movement becomes very small.
//
// Higher threshold = damping starts sooner.
// Lower threshold  = small movement remains responsive longer.
//

#define AIR_MOUSE_STOP_DAMPING_THRESHOLD_X 0.15f
#define AIR_MOUSE_STOP_DAMPING_THRESHOLD_Y 0.25f


// ============================================================================
// AIR MOUSE SPEED ZONES
// ============================================================================
//
// The air mouse uses three movement-speed zones.
//
// Zone 1:
// Small movement, precision control.
//
// Zone 2:
// Medium movement.
//
// Zone 3:
// Large movement, fastest cursor motion.
//
// Threshold examples:
//   Below 4.0  = zone 1
//   4.0-12.0   = zone 2
//   Above 12.0 = zone 3
//
// Multipliers determine the speed in each zone.
//

#define AIR_MOUSE_ZONE_1_THRESHOLD 4.0f
#define AIR_MOUSE_ZONE_2_THRESHOLD 12.0f

#define AIR_MOUSE_ZONE_1_MULTIPLIER_X 1.0f
#define AIR_MOUSE_ZONE_2_MULTIPLIER_X 1.6f
#define AIR_MOUSE_ZONE_3_MULTIPLIER_X 2.4f

#define AIR_MOUSE_ZONE_1_MULTIPLIER_Y 1.0f
#define AIR_MOUSE_ZONE_2_MULTIPLIER_Y 1.35f
#define AIR_MOUSE_ZONE_3_MULTIPLIER_Y 2.0f


// ============================================================================
// AIR MOUSE CLICK AND DRAG STABILIZATION
// ============================================================================
//
// AIR_MOUSE_CLICK_STABILIZE_MS:
// Time after a button press during which cursor movement can be suppressed.
//
// AIR_MOUSE_DRAG_START_PIXELS:
// Amount of intended movement required before a press is treated as dragging.
//

#define AIR_MOUSE_CLICK_STABILIZE_MS 650UL
#define AIR_MOUSE_DRAG_START_PIXELS  80
