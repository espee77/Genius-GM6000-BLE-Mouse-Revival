#include "IrPower.h"
#include "Pins.h"
#include "Config.h"

static bool irState = false;

#if IR_LED_PWM_ENABLED
static uint8_t irDutyPercentToPwmValue(uint8_t dutyPercent) {
    if (dutyPercent >= 100) return 255;
    return (uint8_t)((255UL * dutyPercent) / 100UL);
}
#endif

void irPowerBegin() {
    pinMode(IR_LED_POWER_PIN, OUTPUT);

#if IR_LED_PWM_ENABLED
    analogWriteResolution(IR_PWM_RESOLUTION_BITS);
#endif

    irState = false;
    setIrLeds(true);
}

void setIrLeds(bool on) {
    if (on == irState) return;

    irState = on;

#if IR_LED_PWM_ENABLED
    if (on) {
        analogWrite(IR_LED_POWER_PIN, irDutyPercentToPwmValue(IR_PWM_DUTY_PERCENT));
    } else {
        analogWrite(IR_LED_POWER_PIN, 0);
    }
#else
    digitalWrite(IR_LED_POWER_PIN, on ? HIGH : LOW);
#endif

#if DEBUG_ENABLED && DEBUG_SLEEP
    Serial.print("IR LEDs ");
    Serial.print(on ? "ON" : "OFF");
#if IR_LED_PWM_ENABLED
    if (on) {
        Serial.print(" PWM ");
        Serial.print(IR_PWM_DUTY_PERCENT);
        Serial.print("%");
    }
#endif
    Serial.println();
#endif
}


bool irLedsAreOn() {
    return irState;
}
