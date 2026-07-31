#include "SystemManager.h"
#include "Config.h"
#include "Battery.h"
#include "BleMouse.h"
#include "UsbMouse.h"
#include "PowerManager.h"
#include "UsageStats.h"
#include "Buttons.h"

#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <nrf.h>
#include <string.h>
#include <stddef.h>

using namespace Adafruit_LittleFS_Namespace;

namespace {

constexpr uint32_t SYSTEM_MAGIC = 0x47534D47UL; // GSMG
constexpr uint16_t SYSTEM_FORMAT_VERSION = 3;
constexpr char SYSTEM_FILENAME[] = "/gm6000_system.bin";
constexpr uint8_t EVENT_COUNT = 64;
constexpr uint8_t EVENT_SAVE_BATCH = 10;
constexpr uint32_t EVENT_SAVE_INTERVAL_MS = 30000UL;
constexpr uint8_t EVENT_TEXT_LENGTH = 28;

struct StoredEvent {
    uint32_t bootNumber;
    uint32_t uptimeSeconds;
    char text[EVENT_TEXT_LENGTH];
};

struct PersistentSystemData {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t totalBoots;
    uint32_t watchdogResets;
    uint32_t softwareResets;
    uint32_t lockupResets;
    uint32_t brownoutOrPowerResets;
    uint32_t bleRecoveries;
    uint32_t criticalBatteryShutdowns;
    uint8_t eventWriteIndex;
    uint8_t eventUsed;
    uint8_t lastSelfTestMask;
    uint8_t reserved;
    StoredEvent events[EVENT_COUNT];
    uint32_t crc;
};

PersistentSystemData data = {};
File systemFile(InternalFS);
uint32_t capturedResetReason = 0;
char resetCauseText[48] = "Unknown";
bool storageReady = false;
bool selfTestPassed = false;
bool bootloaderPending = false;
uint32_t bootloaderRequestedAt = 0;
uint32_t lastBleHealthCheckMs = 0;
uint32_t lastCriticalBatteryCheckMs = 0;
bool lastObservedBleConnected = false;
uint8_t criticalBatterySamples = 0;
bool eventLogDirty = false;
uint8_t unsavedEventCount = 0;
uint32_t lastEventSaveMs = 0;

uint32_t crc32(const uint8_t *bytes, size_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t i = 0; i < length; ++i) {
        crc ^= bytes[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
        }
    }
    return ~crc;
}

void resetStoredData()
{
    memset(&data, 0, sizeof(data));
    data.magic = SYSTEM_MAGIC;
    data.version = SYSTEM_FORMAT_VERSION;
    data.size = sizeof(data);
}

bool validStoredData(const PersistentSystemData &candidate)
{
    if (candidate.magic != SYSTEM_MAGIC ||
        candidate.version != SYSTEM_FORMAT_VERSION ||
        candidate.size != sizeof(PersistentSystemData)) {
        return false;
    }

    // CRC covers every byte before the crc field. This avoids creating a
    // second 2+ KB PersistentSystemData object on the small FreeRTOS stack.
    const uint32_t expected = crc32(
        reinterpret_cast<const uint8_t *>(&candidate),
        offsetof(PersistentSystemData, crc)
    );
    return candidate.crc == expected;
}

void saveStoredData()
{
    if (!storageReady) return;

    data.crc = crc32(
        reinterpret_cast<const uint8_t *>(&data),
        offsetof(PersistentSystemData, crc)
    );

    if (systemFile.open(SYSTEM_FILENAME, FILE_O_WRITE)) {
        systemFile.write(reinterpret_cast<const uint8_t *>(&data), sizeof(data));
        systemFile.close();
        eventLogDirty = false;
        unsavedEventCount = 0;
        lastEventSaveMs = millis();
    }
}

void loadStoredData()
{
    // UsageStats owns the single InternalFS.begin() call during startup.
    // Calling begin() again from another module proved unsafe on this BSP.
    storageReady = usageStatsStorageReady();
    if (!storageReady || !systemFile.open(SYSTEM_FILENAME, FILE_O_READ)) {
        resetStoredData();
        return;
    }

    // Read directly into the global buffer. Avoid a second large local copy
    // which can overflow the setup task stack with a 64-entry event log.
    const size_t bytesRead = systemFile.read(
        reinterpret_cast<uint8_t *>(&data),
        sizeof(data)
    );
    systemFile.close();

    if (bytesRead != sizeof(data) || !validStoredData(data)) {
        resetStoredData();
    }
}

void describeResetReason()
{
    if (capturedResetReason & POWER_RESETREAS_DOG_Msk) {
        strncpy(resetCauseText, "Hardware watchdog", sizeof(resetCauseText));
    } else if (capturedResetReason & POWER_RESETREAS_LOCKUP_Msk) {
        strncpy(resetCauseText, "CPU lockup", sizeof(resetCauseText));
    } else if (capturedResetReason & POWER_RESETREAS_SREQ_Msk) {
        strncpy(resetCauseText, "Software reset", sizeof(resetCauseText));
    } else if (capturedResetReason & POWER_RESETREAS_RESETPIN_Msk) {
        strncpy(resetCauseText, "Reset pin / bootloader", sizeof(resetCauseText));
    } else if (capturedResetReason & POWER_RESETREAS_OFF_Msk) {
        strncpy(resetCauseText, "Wake from System OFF", sizeof(resetCauseText));
    } else if (capturedResetReason == 0) {
        strncpy(resetCauseText, "Power-on / brownout", sizeof(resetCauseText));
    } else {
        snprintf(resetCauseText, sizeof(resetCauseText), "Other (0x%08lX)", (unsigned long)capturedResetReason);
    }
    resetCauseText[sizeof(resetCauseText) - 1] = '\0';
}

void startWatchdog()
{
#if SYSTEM_WATCHDOG_ENABLED
    if (NRF_WDT->RUNSTATUS != 0) return;

    NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Pause << WDT_CONFIG_SLEEP_Pos) |
                      (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos);
    NRF_WDT->CRV = (SYSTEM_WATCHDOG_TIMEOUT_MS * 32768UL) / 1000UL;
    NRF_WDT->RREN = WDT_RREN_RR0_Msk;
    NRF_WDT->TASKS_START = 1;
#endif
}

void feedWatchdog()
{
#if SYSTEM_WATCHDOG_ENABLED
    NRF_WDT->RR[0] = WDT_RR_RR_Reload;
#endif
}

void updateBleRecovery()
{
    const uint32_t now = millis();
    if (now - lastBleHealthCheckMs < BLE_SELF_HEAL_INTERVAL_MS) return;
    lastBleHealthCheckMs = now;

    if (usbMouseHostMounted() || bleMouseConnected() || Bluefruit.Advertising.isRunning()) return;

    bleMouseSetAdvertisingAllowed(true);
    bleMouseSetInputEnabled(true);
    bleMouseStartAdvertising();

    if (Bluefruit.Advertising.isRunning()) {
        data.bleRecoveries++;
        systemManagerRecordEvent("BLE advertising recovered");
    }
}

void updateCriticalBattery()
{
    const uint32_t now = millis();
    if (now - lastCriticalBatteryCheckMs < CRITICAL_BATTERY_CHECK_INTERVAL_MS) return;
    lastCriticalBatteryCheckMs = now;

    if (batteryUsbPowerPresent() || batteryIsCharging() || batteryIsFullyCharged()) {
        criticalBatterySamples = 0;
        return;
    }

    const float voltage = batteryVoltage();
    if (voltage > 2.5f && voltage <= CRITICAL_BATTERY_SHUTDOWN_V) {
        if (criticalBatterySamples < 255) criticalBatterySamples++;
    } else if (voltage >= CRITICAL_BATTERY_RECOVERY_V) {
        criticalBatterySamples = 0;
    }

    if (criticalBatterySamples >= CRITICAL_BATTERY_CONFIRM_SAMPLES) {
        data.criticalBatteryShutdowns++;
        systemManagerRecordEvent("Critical battery shutdown");
        saveStoredData();
        usageStatsSave();
        powerEnterCriticalShutdown();
    }
}

} // namespace

void systemManagerEarlyBegin()
{
    capturedResetReason = NRF_POWER->RESETREAS;
    NRF_POWER->RESETREAS = capturedResetReason;
    describeResetReason();
}

void systemManagerBegin(bool imuStartedSuccessfully)
{
    loadStoredData();
    data.totalBoots++;
    lastObservedBleConnected = bleMouseConnected();

    if (capturedResetReason & POWER_RESETREAS_DOG_Msk) data.watchdogResets++;
    if (capturedResetReason & POWER_RESETREAS_SREQ_Msk) data.softwareResets++;
    if (capturedResetReason & POWER_RESETREAS_LOCKUP_Msk) data.lockupResets++;
    if (capturedResetReason == 0) data.brownoutOrPowerResets++;

    uint8_t selfTestMask = 0;
    if (imuStartedSuccessfully) selfTestMask |= 0x01;
    if (batteryRaw() > 0 && batteryVoltage() >= 2.5f && batteryVoltage() <= 4.5f) selfTestMask |= 0x02;
    if (usbMouseHostMounted() || bleMouseConnected() || Bluefruit.Advertising.isRunning()) selfTestMask |= 0x04;
    if (storageReady) selfTestMask |= 0x08;
    // A pressed mouse button is valid after System OFF wake, so only verify
    // that no bits outside the three HID buttons are present.
    if ((buttonsCurrent() & 0xF8) == 0) selfTestMask |= 0x10;

    data.lastSelfTestMask = selfTestMask;
    selfTestPassed = selfTestMask == 0x1F;

    systemManagerRecordEvent(resetCauseText);
    systemManagerRecordEvent(selfTestPassed ? "Self-test passed" : "Self-test warning");
    saveStoredData();
    startWatchdog();
    feedWatchdog();
}

void systemManagerUpdate()
{
    feedWatchdog();
    updateBleRecovery();
    updateCriticalBattery();

    // BLE callbacks may execute in SoftDevice context. Keep persistent event
    // bookkeeping in the normal main loop to avoid concurrent ring-buffer
    // writes while SystemManager is saving or printing the log.
    const bool bleConnectedNow = bleMouseConnected();
    if (bleConnectedNow != lastObservedBleConnected) {
        systemManagerRecordEvent(bleConnectedNow ? "BLE connected" : "BLE disconnected");
        lastObservedBleConnected = bleConnectedNow;
    }

    // All flash writes happen from the main loop, never from BLE callbacks.
    if (eventLogDirty &&
        (unsavedEventCount >= EVENT_SAVE_BATCH ||
         millis() - lastEventSaveMs >= EVENT_SAVE_INTERVAL_MS)) {
        saveStoredData();
    }

    if (bootloaderPending && millis() - bootloaderRequestedAt >= 250) {
        usageStatsSave();
        saveStoredData();
        delay(20);
        NRF_POWER->GPREGRET = 0x57; // Adafruit nRF52 UF2 bootloader request
        NVIC_SystemReset();
    }
}

void systemManagerRecordEvent(const char *eventText)
{
    if (!eventText) return;

    StoredEvent &event = data.events[data.eventWriteIndex];
    event.bootNumber = data.totalBoots;
    event.uptimeSeconds = millis() / 1000UL;
    strncpy(event.text, eventText, sizeof(event.text) - 1);
    event.text[sizeof(event.text) - 1] = '\0';

    data.eventWriteIndex = (data.eventWriteIndex + 1) % EVENT_COUNT;
    if (data.eventUsed < EVENT_COUNT) data.eventUsed++;

    eventLogDirty = true;
    if (unsavedEventCount < 255) unsavedEventCount++;
    // Do not write here: this function may be called from a BLE callback.
    // systemManagerUpdate() performs the deferred save from the main loop.
}

void systemManagerPrintStatus()
{
    Serial.println();
    Serial.println("[SystemManager]");
    Serial.print("Reset cause              : "); Serial.println(resetCauseText);
    Serial.print("Self-test                : "); Serial.println(selfTestPassed ? "PASS" : "CHECK");
    Serial.print("Self-test mask           : 0x"); Serial.println(data.lastSelfTestMask, HEX);
    Serial.print("System boots             : "); Serial.println(data.totalBoots);
    Serial.print("Watchdog resets          : "); Serial.println(data.watchdogResets);
    Serial.print("Software resets          : "); Serial.println(data.softwareResets);
    Serial.print("CPU lockup resets        : "); Serial.println(data.lockupResets);
    Serial.print("Power/brownout boots     : "); Serial.println(data.brownoutOrPowerResets);
    Serial.print("BLE recoveries           : "); Serial.println(data.bleRecoveries);
    Serial.print("Critical batt shutdowns  : "); Serial.println(data.criticalBatteryShutdowns);
    Serial.print("Watchdog                 : "); Serial.println(SYSTEM_WATCHDOG_ENABLED ? "Enabled" : "Disabled");
}

void systemManagerPrintEventLog()
{
    Serial.println();
    Serial.println("================ SYSTEM EVENT LOG ==================");
    if (data.eventUsed == 0) {
        Serial.println("No events stored.");
    } else {
        const uint8_t oldest = (data.eventWriteIndex + EVENT_COUNT - data.eventUsed) % EVENT_COUNT;
        for (uint8_t i = 0; i < data.eventUsed; ++i) {
            const StoredEvent &event = data.events[(oldest + i) % EVENT_COUNT];
            Serial.print("Boot "); Serial.print(event.bootNumber);
            Serial.print(" +"); Serial.print(event.uptimeSeconds); Serial.print("s: ");
            Serial.println(event.text);
        }
    }
    Serial.println("=====================================================");
    Serial.println();
}

void systemManagerFlush()
{
    saveStoredData();
}

void systemManagerClearEventLog()
{
    memset(data.events, 0, sizeof(data.events));
    data.eventWriteIndex = 0;
    data.eventUsed = 0;
    eventLogDirty = true;
    unsavedEventCount = EVENT_SAVE_BATCH;
    saveStoredData();
    Serial.println("Event log cleared.");
}

void systemManagerPrintReport()
{
    Serial.println();
    Serial.println("=====================================================");
    Serial.println("GM-6000 DIAGNOSTIC REPORT");
    Serial.println("=====================================================");
    Serial.print("Firmware                 : "); Serial.println(FIRMWARE_VERSION_STRING);
    Serial.print("Build                    : "); Serial.print(__DATE__); Serial.print(" "); Serial.println(__TIME__);
    Serial.print("Uptime                   : "); Serial.print(millis() / 1000UL); Serial.println(" s");
    Serial.print("Battery                  : "); Serial.print(batteryVoltage(), 2); Serial.print(" V / "); Serial.print(batteryPercent()); Serial.println(" %");
    Serial.print("Charging                 : "); Serial.println(batteryIsCharging() ? "Yes" : "No");
    Serial.print("USB mounted              : "); Serial.println(usbMouseHostMounted() ? "Yes" : "No");
    Serial.print("BLE connected            : "); Serial.println(bleMouseConnected() ? "Yes" : "No");
    Serial.print("BLE advertising          : "); Serial.println(Bluefruit.Advertising.isRunning() ? "Yes" : "No");
    systemManagerPrintStatus();
    usageStatsPrint();
    systemManagerPrintEventLog();
    Serial.println("================ END REPORT ========================");
    Serial.println();
}

void systemManagerRequestBootloader()
{
    if (bootloaderPending) return;
    systemManagerRecordEvent("Bootloader requested");
    Serial.println("Restarting into UF2 bootloader...");
    Serial.flush();
    bootloaderPending = true;
    bootloaderRequestedAt = millis();
}

const char *systemManagerResetCauseText()
{
    return resetCauseText;
}

bool systemManagerSelfTestPassed()
{
    return selfTestPassed;
}


bool systemManagerWasSystemOffWake()
{
    return (capturedResetReason & POWER_RESETREAS_OFF_Msk) != 0;
}
