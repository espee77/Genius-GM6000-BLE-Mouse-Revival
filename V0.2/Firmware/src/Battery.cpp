#include "Battery.h"
#include "Pins.h"
#include "Config.h"
#include "BleMouse.h"

static uint8_t currentBatteryPercent = 0;
static float currentBatteryVoltage = 0.0f;
static float filteredBatteryVoltage = 0.0f;
static uint32_t currentBatteryRaw = 0;
static uint32_t lastBatteryUpdate = 0;

static float readBatteryVoltage() {
    digitalWrite(READ_BAT_ENABLE_PIN, LOW);
    delay(20);

    const uint8_t sampleCount = 8;

    uint32_t rawSum = 0;
    uint16_t rawMin = 4095;
    uint16_t rawMax = 0;

    for (uint8_t i = 0; i < sampleCount; i++) {
        uint16_t raw = analogRead(VBAT_ADC_PIN);

        rawSum += raw;

        if (raw < rawMin) rawMin = raw;
        if (raw > rawMax) rawMax = raw;

        delay(2);
    }

    digitalWrite(READ_BAT_ENABLE_PIN, HIGH);

    uint32_t trimmedSum = rawSum - rawMin - rawMax;
    currentBatteryRaw = trimmedSum / (sampleCount - 2);

    float adcVoltage = ((float)currentBatteryRaw / 4095.0f) * 2.4f;
    return adcVoltage * BATTERY_DIVIDER_FACTOR;
}

static uint8_t batteryVoltageToPercent(float voltage) {
    if (voltage < 1.0f) return 0;
    if (voltage >= 4.20f) return 100;
    if (voltage <= 3.45f) return 0;

    struct Point {
        float voltage;
        uint8_t percent;
    };

    static const Point table[] = {
        {4.20f, 100},
        {4.15f,  95},
        {4.11f,  90},
        {4.08f,  85},
        {4.02f,  80},
        {3.98f,  75},
        {3.95f,  70},
        {3.91f,  65},
        {3.87f,  60},
        {3.85f,  55},
        {3.82f,  50},
        {3.80f,  45},
        {3.79f,  40},
        {3.77f,  35},
        {3.75f,  30},
        {3.73f,  25},
        {3.71f,  20},
        {3.69f,  15},
        {3.61f,  10},
        {3.55f,   5},
        {3.45f,   0}
    };

    const uint8_t count = sizeof(table) / sizeof(table[0]);

    for (uint8_t i = 0; i < count - 1; i++) {
        if (voltage <= table[i].voltage &&
            voltage >= table[i + 1].voltage) {

            float vHigh = table[i].voltage;
            float vLow  = table[i + 1].voltage;
            float pHigh = table[i].percent;
            float pLow  = table[i + 1].percent;

            float percent =
                pLow + ((voltage - vLow) * (pHigh - pLow)) / (vHigh - vLow);

            if (percent < 0.0f) percent = 0.0f;
            if (percent > 100.0f) percent = 100.0f;

            return (uint8_t)(percent + 0.5f);
        }
    }

    return 0;
}

void batteryBegin() {
    pinMode(READ_BAT_ENABLE_PIN, OUTPUT);
    digitalWrite(READ_BAT_ENABLE_PIN, HIGH);

    analogReference(AR_INTERNAL_2_4);
    analogReadResolution(12);

    batteryUpdateNow();
}

void batteryUpdateNow() {
    currentBatteryVoltage = readBatteryVoltage();

    if (filteredBatteryVoltage == 0.0f) {
        filteredBatteryVoltage = currentBatteryVoltage;
    } else {
        // Als de spanning snel stijgt tijdens laden, filter sneller laten volgen
        if (currentBatteryVoltage > filteredBatteryVoltage + 0.10f) {
            filteredBatteryVoltage =
                filteredBatteryVoltage * 0.4f +
                currentBatteryVoltage * 0.6f;
        } else {
            filteredBatteryVoltage =
                filteredBatteryVoltage * 0.8f +
                currentBatteryVoltage * 0.2f;
        }
    }

    uint8_t newPercent = batteryVoltageToPercent(filteredBatteryVoltage);

    bool shouldUpdate = false;

    if (newPercent > currentBatteryPercent) {
        shouldUpdate = true;
    } else if (currentBatteryPercent - newPercent >= 2) {
        shouldUpdate = true;
    }
    currentBatteryPercent = newPercent;
    bleMouseBatteryWrite(currentBatteryPercent);
    
#if DEBUG_ENABLED && DEBUG_BATTERY
    Serial.print("RAW=");
    Serial.print(currentBatteryRaw);
    Serial.print(" voltage=");
    Serial.print(currentBatteryVoltage, 3);
    Serial.print(" filtered=");
    Serial.print(filteredBatteryVoltage, 3);
    Serial.print(" percent=");
    Serial.println(currentBatteryPercent);
#endif
}

void batteryUpdate() {
    if (millis() - lastBatteryUpdate < BATTERY_UPDATE_INTERVAL_MS) {
        return;
    }

    lastBatteryUpdate = millis();
    batteryUpdateNow();
}

uint8_t batteryPercent() {
    return currentBatteryPercent;
}

float batteryVoltage() {
    return currentBatteryVoltage;
}

uint32_t batteryRaw() {
    return currentBatteryRaw;
}