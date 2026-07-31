#include "Battery.h"
#include "Pins.h"
#include "Config.h"
#include "BleMouse.h"

#include "nrf.h"
#include "nrf_gpio.h"

static uint8_t currentBatteryPercent = 0;
static float currentBatteryVoltage = 0.0f;
static float filteredBatteryVoltage = 0.0f;
static uint32_t currentBatteryRaw = 0;

static uint32_t lastBatteryUpdate = 0;
static uint32_t lastChargeStatusPoll = 0;

static bool batteryPercentInitialized = false;

static BatteryChargeStatus currentChargeStatus =
    BatteryChargeStatus::OnBattery;

static BatteryChargeStatus pendingChargeStatus =
    BatteryChargeStatus::OnBattery;

static uint32_t pendingChargeStatusSince = 0;

static bool usbVbusPresent()
{
    return (NRF_POWER->USBREGSTATUS &
            POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0;
}

static bool chargerActiveLow()
{
    // BQ25101 ~CHG is open-drain and active LOW.
    return nrf_gpio_pin_read(CHARGE_STATUS_NRF_PIN) == 0;
}

static BatteryChargeStatus readRawChargeStatus()
{
    if (!usbVbusPresent()) {
        return BatteryChargeStatus::OnBattery;
    }

    return chargerActiveLow()
        ? BatteryChargeStatus::Charging
        : BatteryChargeStatus::FullyCharged;
}

static uint8_t batteryPowerStateValue()
{
    // Bluetooth Battery Power State (UUID 0x2A1A):
    // bits 0-1: battery present       = 3
    // bits 2-3: discharging state
    // bits 4-5: charging state
    // bits 6-7: level state
    const uint8_t present = 3;

    const uint8_t discharging =
        currentChargeStatus == BatteryChargeStatus::OnBattery ? 3 : 2;

    const uint8_t charging =
        currentChargeStatus == BatteryChargeStatus::Charging ? 3 : 2;

    const uint8_t level =
        currentBatteryPercent <= 5 ? 3 : 2;

    return present |
           (discharging << 2) |
           (charging << 4) |
           (level << 6);
}

static void publishBatteryState()
{
    bleMouseBatteryWrite(currentBatteryPercent);
    bleMouseBatteryPowerStateWrite(batteryPowerStateValue());
}

static void updateChargeStatus()
{
    const uint32_t now = millis();

    if (now - lastChargeStatusPoll < CHARGE_STATUS_POLL_INTERVAL_MS) {
        return;
    }

    lastChargeStatusPoll = now;

    const BatteryChargeStatus rawStatus = readRawChargeStatus();

    // Seeed warns not to drive D14 HIGH while USB charging power is present.
    // Keep the battery divider path enabled for the whole USB-powered period.
    digitalWrite(
        READ_BAT_ENABLE_PIN,
        usbVbusPresent() ? LOW : HIGH
    );

    if (rawStatus != pendingChargeStatus) {
        pendingChargeStatus = rawStatus;
        pendingChargeStatusSince = now;
        return;
    }

    if (rawStatus == currentChargeStatus) {
        return;
    }

    if (now - pendingChargeStatusSince < CHARGE_STATUS_DEBOUNCE_MS) {
        return;
    }

    currentChargeStatus = rawStatus;

    if (currentChargeStatus == BatteryChargeStatus::FullyCharged) {
        currentBatteryPercent = 100;
        batteryPercentInitialized = true;
    }

    publishBatteryState();

#if DEBUG_ENABLED && DEBUG_BATTERY
    Serial.print("Charge status: ");
    Serial.println(batteryChargeStatusText());
#endif
}

static float readBatteryVoltage()
{
    // LOW enables the on-board battery divider.
    digitalWrite(READ_BAT_ENABLE_PIN, LOW);
    delay(20);

    const uint8_t sampleCount = 8;

    uint32_t rawSum = 0;
    uint16_t rawMin = 4095;
    uint16_t rawMax = 0;

    for (uint8_t i = 0; i < sampleCount; i++) {
        const uint16_t raw = analogRead(VBAT_ADC_PIN);

        rawSum += raw;

        if (raw < rawMin) rawMin = raw;
        if (raw > rawMax) rawMax = raw;

        delay(2);
    }

    // On battery, disable the divider after measuring to save power.
    // With USB/VBUS present it must remain LOW for P0.31 safety.
    digitalWrite(
        READ_BAT_ENABLE_PIN,
        usbVbusPresent() ? LOW : HIGH
    );

    const uint32_t trimmedSum = rawSum - rawMin - rawMax;
    currentBatteryRaw = trimmedSum / (sampleCount - 2);

    const float adcVoltage =
        ((float)currentBatteryRaw / 4095.0f) * 2.4f;

    return adcVoltage * BATTERY_DIVIDER_FACTOR;
}

static uint8_t batteryVoltageToPercent(float voltage)
{
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

            const float vHigh = table[i].voltage;
            const float vLow = table[i + 1].voltage;
            const float pHigh = table[i].percent;
            const float pLow = table[i + 1].percent;

            float percent =
                pLow +
                ((voltage - vLow) * (pHigh - pLow)) /
                (vHigh - vLow);

            if (percent < 0.0f) percent = 0.0f;
            if (percent > 100.0f) percent = 100.0f;

            return (uint8_t)(percent + 0.5f);
        }
    }

    return 0;
}

void batteryBegin()
{
    nrf_gpio_cfg_input(
        CHARGE_STATUS_NRF_PIN,
        NRF_GPIO_PIN_PULLUP
    );

    pinMode(READ_BAT_ENABLE_PIN, OUTPUT);

    // Keep D14 LOW from the start whenever USB power is present.
    digitalWrite(
        READ_BAT_ENABLE_PIN,
        usbVbusPresent() ? LOW : HIGH
    );

    analogReference(AR_INTERNAL_2_4);
    analogReadResolution(12);

    // Do not immediately trust a HIGH ~CHG level at USB power-up:
    // the charger output can briefly be inactive before charging starts.
    currentChargeStatus = BatteryChargeStatus::OnBattery;
    pendingChargeStatus = readRawChargeStatus();
    pendingChargeStatusSince = millis();

    batteryUpdateNow();

    lastBatteryUpdate = millis();
    lastChargeStatusPoll = millis();

#if DEBUG_ENABLED && DEBUG_BATTERY
    Serial.print("Initial charge status: ");
    Serial.println(batteryChargeStatusText());
#endif
}

void batteryUpdateNow()
{
    currentBatteryVoltage = readBatteryVoltage();

    if (filteredBatteryVoltage == 0.0f) {
        filteredBatteryVoltage = currentBatteryVoltage;
    } else {
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

    const uint8_t newPercent =
        batteryVoltageToPercent(filteredBatteryVoltage);

    bool shouldUpdate = !batteryPercentInitialized;

    if (currentChargeStatus == BatteryChargeStatus::FullyCharged) {
        shouldUpdate = !batteryPercentInitialized ||
                       currentBatteryPercent != 100;
        currentBatteryPercent = 100;
    } else if (batteryPercentInitialized) {
        if (newPercent > currentBatteryPercent) {
            shouldUpdate = true;
        } else if (newPercent + 2 <= currentBatteryPercent) {
            shouldUpdate = true;
        }
    }

    if (shouldUpdate) {
        if (currentChargeStatus != BatteryChargeStatus::FullyCharged) {
            currentBatteryPercent = newPercent;
        }

        batteryPercentInitialized = true;
        publishBatteryState();
    }

#if DEBUG_ENABLED && DEBUG_BATTERY
    Serial.print("RAW=");
    Serial.print(currentBatteryRaw);
    Serial.print(" voltage=");
    Serial.print(currentBatteryVoltage, 3);
    Serial.print(" filtered=");
    Serial.print(filteredBatteryVoltage, 3);
    Serial.print(" measured=");
    Serial.print(newPercent);
    Serial.print("% published=");
    Serial.print(currentBatteryPercent);
    Serial.print("% charge=");
    Serial.println(batteryChargeStatusText());
#endif
}

void batteryUpdate()
{
    updateChargeStatus();

    if (millis() - lastBatteryUpdate < BATTERY_UPDATE_INTERVAL_MS) {
        return;
    }

    lastBatteryUpdate = millis();
    batteryUpdateNow();
}

uint8_t batteryPercent()
{
    return currentBatteryPercent;
}

float batteryVoltage()
{
    return currentBatteryVoltage;
}

uint32_t batteryRaw()
{
    return currentBatteryRaw;
}

bool batteryUsbPowerPresent()
{
    return usbVbusPresent();
}

bool batteryIsCharging()
{
    return currentChargeStatus == BatteryChargeStatus::Charging;
}

bool batteryIsFullyCharged()
{
    return currentChargeStatus == BatteryChargeStatus::FullyCharged;
}

const char* batteryChargeStatusText()
{
    switch (currentChargeStatus) {
        case BatteryChargeStatus::Charging:
            return "Charging";

        case BatteryChargeStatus::FullyCharged:
            return "Fully charged";

        case BatteryChargeStatus::OnBattery:
        default:
            return "On battery";
    }
}
