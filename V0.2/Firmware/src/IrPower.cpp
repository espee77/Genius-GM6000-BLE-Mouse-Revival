#include "IrPower.h"
#include "Pins.h"

void irPowerBegin() {
    pinMode(IR_LED_POWER_PIN, OUTPUT);
    setIrLeds(true);
}

void setIrLeds(bool on) {
    digitalWrite(IR_LED_POWER_PIN, on ? HIGH : LOW);
}
