#include "UsageStats.h"
#include "Config.h"
#include "Battery.h"
#include "BleMouse.h"
#include "UsbMouse.h"
#include "PowerManager.h"
#include "IrPower.h"
#include "ImuManager.h"
#include "SystemManager.h"

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <string.h>
#include <math.h>

using namespace Adafruit_LittleFS_Namespace;

namespace {

constexpr uint32_t STATS_MAGIC = 0x474D5354UL; // GMST
constexpr uint16_t STATS_FORMAT_VERSION = 1;
constexpr char STATS_FILENAME[] = "/gm6000_stats.bin";
constexpr uint32_t SAVE_INTERVAL_MS = 3600000UL;
constexpr uint32_t UPDATE_INTERVAL_MS = 1000UL;

struct PersistentStats {
    uint32_t magic;
    uint16_t formatVersion;
    uint16_t size;

    uint32_t bootCount;
    uint64_t runtimeMs;
    uint64_t activeMs;
    uint64_t idleMs;
    uint64_t bleModeMs;
    uint64_t usbModeMs;
    uint64_t bleConnectedMs;
    uint64_t irOnMs;

    uint32_t idleEntries;
    uint32_t wakeCount;
    uint32_t imuWakes;
    uint32_t movementWakes;
    uint32_t leftWakes;
    uint32_t rightWakes;
    uint32_t middleWakes;
    uint32_t multiHostWakes;

    uint32_t buttonPresses;
    uint32_t leftPresses;
    uint32_t rightPresses;
    uint32_t middlePresses;
    uint32_t multiHostPresses;

    int64_t encoderXSteps;
    int64_t encoderYSteps;
    uint64_t mouseReports;
    uint64_t bleReports;
    uint64_t usbReports;

    uint32_t bleConnects;
    uint32_t bleDisconnects;
    uint32_t outputModeChanges;

    uint32_t chargingSessions;
    uint32_t fullChargeEvents;
    uint32_t chargedPercentTenths;

    float batteryMinV;
    float batteryMaxV;
    float lastBatteryV;
    uint8_t lastBatteryPercent;
    uint8_t reservedA[3];

    uint64_t latestNormalizedDischargeMs;
    uint64_t referenceNormalizedDischargeMs;
    uint32_t completedDischargeSamples;

    char lastWakeReason[20];
    uint32_t crc;
};

PersistentStats stats = {};
File statsFile(InternalFS);

uint32_t lastUpdateMs = 0;
uint32_t lastSaveMs = 0;

// Runtime-only BLE diagnostics. These values are intended for the USB
// DEBUG command and do not add extra flash writes.
uint32_t bleSessionStartMs = 0;
uint32_t lastBleSessionDurationMs = 0;
uint32_t lastBleEventMs = 0;
uint8_t lastBleDisconnectReason = 0;
bool lastBleDisconnectReasonValid = false;
bool bleWasConnectedWhenUsbTookPriority = false;
bool storageReady = false;
bool serialWasOpen = false;
enum class PendingConfirmation : uint8_t {
    None,
    StatsReset,
    EventsClear
};

PendingConfirmation pendingConfirmation = PendingConfirmation::None;
String commandBuffer;

bool lastCharging = false;
bool lastFullyCharged = false;
uint8_t lastChargePercent = 0;

bool dischargeTracking = false;
uint8_t dischargeStartPercent = 0;
uint64_t dischargeStartRuntimeMs = 0;

uint32_t crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
        }
    }
    return ~crc;
}

void finalizeCrc(PersistentStats &value)
{
    value.crc = 0;
    value.crc = crc32(
        reinterpret_cast<const uint8_t *>(&value),
        sizeof(PersistentStats)
    );
}

bool validStats(const PersistentStats &value)
{
    if (value.magic != STATS_MAGIC ||
        value.formatVersion != STATS_FORMAT_VERSION ||
        value.size != sizeof(PersistentStats)) {
        return false;
    }

    PersistentStats copy = value;
    const uint32_t storedCrc = copy.crc;
    copy.crc = 0;
    return storedCrc == crc32(
        reinterpret_cast<const uint8_t *>(&copy),
        sizeof(PersistentStats)
    );
}

void resetStatsData(bool preserveBootCount)
{
    const uint32_t boots = preserveBootCount ? stats.bootCount : 0;
    memset(&stats, 0, sizeof(stats));
    stats.magic = STATS_MAGIC;
    stats.formatVersion = STATS_FORMAT_VERSION;
    stats.size = sizeof(PersistentStats);
    stats.bootCount = boots;
    stats.batteryMinV = 99.0f;
    strncpy(stats.lastWakeReason, "POWER ON", sizeof(stats.lastWakeReason) - 1);
}

bool loadStats()
{
    if (!storageReady) return false;

    if (!statsFile.open(STATS_FILENAME, FILE_O_READ)) {
        return false;
    }

    PersistentStats loaded = {};
    const size_t bytesRead = statsFile.read(
        reinterpret_cast<uint8_t *>(&loaded),
        sizeof(loaded)
    );
    statsFile.close();

    if (bytesRead != sizeof(loaded) || !validStats(loaded)) {
        return false;
    }

    stats = loaded;
    return true;
}

void printDuration(uint64_t milliseconds)
{
    const uint64_t totalSeconds = milliseconds / 1000ULL;
    const uint32_t days = totalSeconds / 86400ULL;
    const uint8_t hours = (totalSeconds % 86400ULL) / 3600ULL;
    const uint8_t minutes = (totalSeconds % 3600ULL) / 60ULL;

    if (days > 0) {
        Serial.print(days);
        Serial.print(" d ");
    }
    Serial.print(hours);
    Serial.print(" h ");
    Serial.print(minutes);
    Serial.print(" min");
}

void printInt64(int64_t value)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%lld", (long long)value);
    Serial.print(buffer);
}

void printUint64(uint64_t value)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%llu", (unsigned long long)value);
    Serial.print(buffer);
}

void printHealth()
{
    Serial.print("Estimated health        : ");
    if (stats.completedDischargeSamples == 0 ||
        stats.referenceNormalizedDischargeMs == 0 ||
        stats.latestNormalizedDischargeMs == 0) {
        Serial.println("Not enough data");
        Serial.println("Health confidence       : None");
        return;
    }

    float health =
        100.0f * (float)stats.latestNormalizedDischargeMs /
        (float)stats.referenceNormalizedDischargeMs;

    if (health > 100.0f) health = 100.0f;
    if (health < 0.0f) health = 0.0f;

    Serial.print(health, 1);
    Serial.println("% (estimated)");

    Serial.print("Health confidence       : ");
    if (stats.completedDischargeSamples >= 6) {
        Serial.println("High");
    } else if (stats.completedDischargeSamples >= 3) {
        Serial.println("Medium");
    } else {
        Serial.println("Low");
    }
}


const char *bleDisconnectReasonText(uint8_t reason)
{
    switch (reason) {
        case 0x08: return "Connection timeout";
        case 0x13: return "Remote user terminated";
        case 0x16: return "Local host terminated";
        case 0x3B: return "Unacceptable connection parameters";
        default:   return "Other / unknown";
    }
}

void printDebug()
{
    const bool usbMode = usbMouseHostMounted();
    const bool bleConnected = bleMouseConnected();

    Serial.println();
    Serial.println("================ GM-6000 DEBUG =====================");

    Serial.println("\n[Current USB and hardware status]");
    Serial.print("Firmware                 : "); Serial.println(FIRMWARE_VERSION_STRING);
    Serial.print("Build                    : "); Serial.print(__DATE__); Serial.print(" "); Serial.println(__TIME__);
    Serial.print("Session uptime           : "); printDuration(millis()); Serial.println();
    Serial.print("Active mouse output      : "); Serial.println(usbMode ? "USB HID" : "BLE HID");
    Serial.print("USB mouse host mounted   : "); Serial.println(usbMode ? "Yes" : "No");
    Serial.print("USB power present        : "); Serial.println(batteryUsbPowerPresent() ? "Yes" : "No");

    Serial.println("\n[Power]");
    Serial.print("Power state              : "); Serial.println(powerIsIdleSleep() ? "IDLE" : "ACTIVE");
    Serial.print("IR encoder LEDs          : "); Serial.println(irLedsAreOn() ? "ON" : "OFF");

    Serial.println("\n[Battery]");
    Serial.print("ADC raw                  : "); Serial.println(batteryRaw());
    Serial.print("Voltage                  : "); Serial.print(batteryVoltage(), 3); Serial.println(" V");
    Serial.print("Percentage               : "); Serial.print(batteryPercent()); Serial.println("%");
    Serial.print("Charge state             : "); Serial.println(batteryChargeStatusText());

    Serial.println("\n[IMU]");
    Serial.print("Acceleration X           : "); Serial.print(imuAccelX(), 4); Serial.println(" g");
    Serial.print("Acceleration Y           : "); Serial.print(imuAccelY(), 4); Serial.println(" g");
    Serial.print("Acceleration Z           : "); Serial.print(imuAccelZ(), 4); Serial.println(" g");
    Serial.print("Gyroscope X              : "); Serial.print(imuGyroX(), 3); Serial.println(" dps");
    Serial.print("Gyroscope Z              : "); Serial.print(imuGyroZ(), 3); Serial.println(" dps");
    Serial.print("Mouse on back            : "); Serial.println(imuIsMouseOnBack() ? "Yes" : "No");
    Serial.print("Air-mouse side           : "); Serial.println(imuIsAirMouseSide() ? "Yes" : "No");
    Serial.print("Last wake reason         : "); Serial.println(stats.lastWakeReason);

    Serial.println("\n[BLE status / last known session]");
    if (usbMode) {
        Serial.println("BLE mouse input          : Disabled while USB HID is active");
        Serial.print("Background BLE link      : "); Serial.println(bleConnected ? "Connected" : "Not connected");
        Serial.print("Advertising              : "); Serial.println(Bluefruit.Advertising.isRunning() ? "Yes" : "No");
        Serial.print("Connected at USB switch  : "); Serial.println(bleWasConnectedWhenUsbTookPriority ? "Yes" : "No");
    } else {
        Serial.print("BLE mouse input          : "); Serial.println(bleMouseInputEnabled() ? "Enabled" : "Disabled");
        Serial.print("BLE link                 : "); Serial.println(bleConnected ? "Connected" : "Not connected");
        Serial.print("Advertising              : "); Serial.println(Bluefruit.Advertising.isRunning() ? "Yes" : "No");
    }

    uint32_t sessionDuration = lastBleSessionDurationMs;
    if (bleConnected && bleSessionStartMs != 0) {
        sessionDuration = millis() - bleSessionStartMs;
    }
    Serial.print("Last/current session     : "); printDuration(sessionDuration); Serial.println();
    Serial.print("Last BLE event           : ");
    if (lastBleEventMs == 0) Serial.println("No event this boot");
    else { printDuration(millis() - lastBleEventMs); Serial.println(" ago"); }
    Serial.print("Last disconnect reason   : ");
    if (!lastBleDisconnectReasonValid) Serial.println("None recorded this boot");
    else {
        Serial.print(bleDisconnectReasonText(lastBleDisconnectReason));
        Serial.print(" (0x");
        if (lastBleDisconnectReason < 0x10) Serial.print('0');
        Serial.print(lastBleDisconnectReason, HEX);
        Serial.println(")");
    }
    Serial.print("Configured BLE profile   : "); Serial.println(bleMouseIdleProfileRequested() ? "IDLE requested" : "ACTIVE");
    Serial.print("Configured interval      : ");
    if (bleMouseIdleProfileRequested()) {
        Serial.print(BLE_IDLE_CONN_INTERVAL * 1.25f, 2);
    } else {
        Serial.print(BLE_ACTIVE_CONN_INTERVAL * 1.25f, 2);
    }
    Serial.println(" ms");
    Serial.print("Configured TX power      : +"); Serial.print(BLE_TX_POWER); Serial.println(" dBm");
    Serial.println("Live RSSI                : Unavailable through USB diagnostics");

    Serial.println("\n[Storage]");
    Serial.print("Statistics storage       : "); Serial.println(storageReady ? "OK" : "Unavailable");
    Serial.print("Statistics record size   : "); Serial.print(sizeof(PersistentStats)); Serial.println(" bytes");
    Serial.print("Time since flash save    : "); printDuration(millis() - lastSaveMs); Serial.println();

    Serial.println("\n[Basic self-test]");
    Serial.print("Battery ADC              : ");
    Serial.println((batteryRaw() > 0 && batteryVoltage() >= 2.5f && batteryVoltage() <= 4.5f) ? "OK" : "CHECK");
    Serial.print("IMU data                 : ");
    const float accelSum = fabsf(imuAccelX()) + fabsf(imuAccelY()) + fabsf(imuAccelZ());
    Serial.println(accelSum > 0.1f ? "OK" : "CHECK");
    Serial.print("USB HID                  : "); Serial.println(usbMode ? "ACTIVE" : "Not active");
    Serial.print("BLE stack                : ");
    Serial.println((bleConnected || Bluefruit.Advertising.isRunning()) ? "OK" : "Not currently active");
    Serial.print("Statistics flash         : "); Serial.println(storageReady ? "OK" : "CHECK");

    Serial.println("=====================================================");
    Serial.println();
}

void printStats()
{
    Serial.println();
    Serial.println("================ GM-6000 STATISTICS ================");

    Serial.println("\n[Current status]");
    Serial.print("Current mode             : "); Serial.println(usbMouseHostMounted() ? "USB" : "BLE");
    Serial.print("Battery                  : "); Serial.print(batteryVoltage(), 3); Serial.print(" V / "); Serial.print(batteryPercent()); Serial.println("%");
    Serial.print("Session uptime           : "); printDuration(millis()); Serial.println();

    Serial.println("\n[Firmware and system]");
    Serial.print("Firmware                 : ");
    Serial.println(FIRMWARE_VERSION_STRING);
    Serial.print("Hardware                 : ");
    Serial.println(FIRMWARE_HARDWARE_REV);
    Serial.print("Boot count               : ");
    Serial.println(stats.bootCount);
    Serial.print("Last wake reason         : ");
    Serial.println(stats.lastWakeReason);
    Serial.print("Last reset cause         : ");
    Serial.println(systemManagerResetCauseText());
    Serial.print("Startup self-test        : ");
    Serial.println(systemManagerSelfTestPassed() ? "PASS" : "CHECK");

    Serial.println("\n[Time by state]");
    Serial.print("Runtime total            : "); printDuration(stats.runtimeMs); Serial.println();
    Serial.print("Active time              : "); printDuration(stats.activeMs); Serial.println();
    Serial.print("Idle time                : "); printDuration(stats.idleMs); Serial.println();
    Serial.print("BLE mode time            : "); printDuration(stats.bleModeMs); Serial.println();
    Serial.print("USB mode time            : "); printDuration(stats.usbModeMs); Serial.println();
    Serial.print("BLE connected time       : "); printDuration(stats.bleConnectedMs); Serial.println();
    Serial.print("IR LED ON time           : "); printDuration(stats.irOnMs); Serial.println();
    Serial.print("Idle entries             : "); Serial.println(stats.idleEntries);
    Serial.print("Wakes from idle          : "); Serial.println(stats.wakeCount);

    Serial.println("\n[Wake and input]");
    Serial.print("IMU wakes                : "); Serial.println(stats.imuWakes);
    Serial.print("Movement wakes           : "); Serial.println(stats.movementWakes);
    Serial.print("Left-button wakes        : "); Serial.println(stats.leftWakes);
    Serial.print("Right-button wakes       : "); Serial.println(stats.rightWakes);
    Serial.print("Middle-button wakes      : "); Serial.println(stats.middleWakes);
    Serial.print("Multi-host wakes         : "); Serial.println(stats.multiHostWakes);
    Serial.print("Button presses total     : "); Serial.println(stats.buttonPresses);
    Serial.print("Encoder X steps          : "); printInt64(stats.encoderXSteps); Serial.println();
    Serial.print("Encoder Y steps          : "); printInt64(stats.encoderYSteps); Serial.println();
    Serial.print("Mouse reports total      : "); printUint64(stats.mouseReports); Serial.println();
    Serial.print("BLE reports              : "); printUint64(stats.bleReports); Serial.println();
    Serial.print("USB reports              : "); printUint64(stats.usbReports); Serial.println();

    Serial.println("\n[Connections]");
    Serial.print("BLE connects             : "); Serial.println(stats.bleConnects);
    Serial.print("BLE disconnects          : "); Serial.println(stats.bleDisconnects);
    Serial.print("BLE/USB mode changes     : "); Serial.println(stats.outputModeChanges);

    Serial.println("\n[Battery and charging]");
    Serial.print("Battery now              : ");
    Serial.print(batteryVoltage(), 3);
    Serial.print(" V / ");
    Serial.print(batteryPercent());
    Serial.print("% / ");
    Serial.println(batteryChargeStatusText());
    Serial.print("Battery lowest           : ");
    if (stats.batteryMinV > 10.0f) Serial.println("No valid sample");
    else { Serial.print(stats.batteryMinV, 3); Serial.println(" V"); }
    Serial.print("Battery highest          : "); Serial.print(stats.batteryMaxV, 3); Serial.println(" V");
    Serial.print("Charging sessions        : "); Serial.println(stats.chargingSessions);
    Serial.print("Full-charge events       : "); Serial.println(stats.fullChargeEvents);
    Serial.print("Charged percentage total : "); Serial.print(stats.chargedPercentTenths / 10.0f, 1); Serial.println("%");
    Serial.print("Equivalent full cycles   : "); Serial.println(stats.chargedPercentTenths / 1000.0f, 2);

    Serial.println("\n[Estimated battery health]");
    printHealth();
    Serial.print("Discharge samples        : "); Serial.println(stats.completedDischargeSamples);
    Serial.print("Latest normalized run    : "); printDuration(stats.latestNormalizedDischargeMs); Serial.println();
    Serial.print("Reference normalized run : "); printDuration(stats.referenceNormalizedDischargeMs); Serial.println();

    Serial.println("=====================================================");
    Serial.println();
}

void printHelp()
{
    Serial.println();
    Serial.println("GM-6000 BLE Mouse");
    Serial.println();
    Serial.println("Available commands:");
    Serial.println();
    Serial.println("HELP         Show this help");
    Serial.println("STATS         Show usage statistics");
    Serial.println("STATS RESET   Erase stored statistics (confirmation required)");
    Serial.println("DEBUG         Show live diagnostics");
    Serial.println("VERSION       Show firmware version");
    Serial.println("SYSTEM        Show reset, watchdog and self-test status");
    Serial.println("EVENTS        Show persistent system event log");
    Serial.println("EVENTS CLEAR  Clear only the event log");
    Serial.println("REPORT        Show complete diagnostic report");
    Serial.println("BOOTLOADER    Restart into the UF2 bootloader");
    Serial.println();
}

void handleCommand(String command)
{
    command.trim();
    command.toUpperCase();
    if (command.length() == 0) return;

    if (command == "STATS") {
        usageStatsPrint();
    } else if (command == "STATS RESET") {
        pendingConfirmation = PendingConfirmation::StatsReset;
        Serial.println("Type CONFIRM STATS RESET to erase all stored statistics.");
    } else if (command == "DEBUG") {
        printDebug();
    } else if (command == "VERSION") {
        Serial.println();
        Serial.println("GM-6000 BLE Mouse");
        Serial.println();
        Serial.print("Firmware : ");
        Serial.println(FIRMWARE_VERSION_STRING);
        Serial.print("Build    : ");
        Serial.print(__DATE__);
        Serial.print(" ");
        Serial.println(__TIME__);
        Serial.println();
    } else if (command == "SYSTEM") {
        systemManagerPrintStatus();
    } else if (command == "EVENTS") {
        systemManagerPrintEventLog();
    } else if (command == "EVENTS CLEAR") {
        pendingConfirmation = PendingConfirmation::EventsClear;
        Serial.println("Type CONFIRM EVENTS CLEAR to erase the event log.");
    } else if (command == "REPORT") {
        systemManagerPrintReport();
    } else if (command == "BOOTLOADER") {
        systemManagerRequestBootloader();

    } else if (command == "CONFIRM STATS RESET") {
        if (pendingConfirmation != PendingConfirmation::StatsReset) {
            Serial.println("No statistics reset is pending. Type STATS RESET first.");
            return;
        }
        pendingConfirmation = PendingConfirmation::None;
        usageStatsReset();
    } else if (command == "CONFIRM EVENTS CLEAR") {
        if (pendingConfirmation != PendingConfirmation::EventsClear) {
            Serial.println("No event-log clear is pending. Type EVENTS CLEAR first.");
            return;
        }
        pendingConfirmation = PendingConfirmation::None;
        systemManagerClearEventLog();
    } else if (command == "HELP") {
        printHelp();
    } else {
        Serial.println("Unknown command. Type HELP.");
    }
}

} // namespace

void usageStatsBegin()
{
#if USAGE_STATS_ENABLED
    Serial.begin(115200);

    storageReady = InternalFS.begin();
    if (!loadStats()) {
        resetStatsData(false);
    }

    stats.bootCount++;
    strncpy(stats.lastWakeReason, "POWER ON", sizeof(stats.lastWakeReason) - 1);
    stats.lastWakeReason[sizeof(stats.lastWakeReason) - 1] = '\0';

    lastUpdateMs = millis();
    lastSaveMs = millis();
    lastCharging = batteryIsCharging();
    lastFullyCharged = batteryIsFullyCharged();
    lastChargePercent = batteryPercent();

    usageStatsRecordBattery(
        batteryVoltage(),
        batteryPercent(),
        lastCharging,
        lastFullyCharged
    );

    usageStatsSave();
#endif
}

void usageStatsUpdate(bool idle, bool usbMode, bool irOn, bool bleConnected)
{
#if USAGE_STATS_ENABLED
    const uint32_t now = millis();
    const uint32_t elapsed = now - lastUpdateMs;

    if (elapsed >= UPDATE_INTERVAL_MS) {
        lastUpdateMs = now;
        stats.runtimeMs += elapsed;

        if (idle) stats.idleMs += elapsed;
        else stats.activeMs += elapsed;

        if (usbMode) stats.usbModeMs += elapsed;
        else stats.bleModeMs += elapsed;

        if (bleConnected) stats.bleConnectedMs += elapsed;
        if (irOn) stats.irOnMs += elapsed;
    }

    usageStatsRecordBattery(
        batteryVoltage(),
        batteryPercent(),
        batteryIsCharging(),
        batteryIsFullyCharged()
    );

    if (now - lastSaveMs >= SAVE_INTERVAL_MS) {
        usageStatsSave();
    }
#else
    (void)idle; (void)usbMode; (void)irOn; (void)bleConnected;
#endif
}

void usageStatsProcessSerial()
{
#if USAGE_STATS_ENABLED
    const bool serialOpen = (bool)Serial;
    if (serialOpen && !serialWasOpen) {
        Serial.println();
        Serial.println("GM-6000 BLE Mouse statistics console");
        Serial.print("Firmware ");
        Serial.println(FIRMWARE_VERSION_STRING);
        Serial.println("Type HELP for commands.");
        Serial.println();
    }
    serialWasOpen = serialOpen;

    while (Serial.available()) {
        const char c = (char)Serial.read();
        if (c == '\r' || c == '\n') {
            if (commandBuffer.length() > 0) {
                handleCommand(commandBuffer);
                commandBuffer = "";
            }
        } else if (commandBuffer.length() < 48) {
            commandBuffer += c;
        }
    }
#endif
}

void usageStatsPrint()
{
#if USAGE_STATS_ENABLED
    printStats();
#endif
}

void usageStatsReset()
{
#if USAGE_STATS_ENABLED
    const uint32_t currentBoot = stats.bootCount;
    resetStatsData(false);
    stats.bootCount = currentBoot;
    strncpy(stats.lastWakeReason, "STATS RESET", sizeof(stats.lastWakeReason) - 1);
    stats.lastWakeReason[sizeof(stats.lastWakeReason) - 1] = '\0';
    usageStatsSave();
    Serial.println("Statistics reset.");
#endif
}

void usageStatsSave()
{
#if USAGE_STATS_ENABLED
    if (!storageReady) return;

    finalizeCrc(stats);

    if (statsFile.open(STATS_FILENAME, FILE_O_WRITE)) {
        statsFile.write(
            reinterpret_cast<const uint8_t *>(&stats),
            sizeof(stats)
        );
        statsFile.close();
        lastSaveMs = millis();
    }
#endif
}

void usageStatsRecordWake(const char *reason, bool fromIdle)
{
#if USAGE_STATS_ENABLED
    if (reason) {
        strncpy(stats.lastWakeReason, reason, sizeof(stats.lastWakeReason) - 1);
        stats.lastWakeReason[sizeof(stats.lastWakeReason) - 1] = '\0';
    }

    if (!fromIdle) return;

    stats.wakeCount++;
    if (!reason) return;
    if (strcmp(reason, "IMU") == 0) stats.imuWakes++;
    else if (strcmp(reason, "MOVEMENT") == 0) stats.movementWakes++;
    else if (strcmp(reason, "LEFT") == 0) stats.leftWakes++;
    else if (strcmp(reason, "RIGHT") == 0) stats.rightWakes++;
    else if (strcmp(reason, "MIDDLE") == 0) stats.middleWakes++;
    else if (strcmp(reason, "MULTI-HOST") == 0) stats.multiHostWakes++;
#else
    (void)reason; (void)fromIdle;
#endif
}

void usageStatsRecordButtonPress(const char *buttonName)
{
#if USAGE_STATS_ENABLED
    stats.buttonPresses++;
    if (!buttonName) return;
    if (strcmp(buttonName, "LEFT") == 0) stats.leftPresses++;
    else if (strcmp(buttonName, "RIGHT") == 0) stats.rightPresses++;
    else if (strcmp(buttonName, "MIDDLE") == 0) stats.middlePresses++;
    else if (strcmp(buttonName, "MULTI-HOST") == 0) stats.multiHostPresses++;
#else
    (void)buttonName;
#endif
}

void usageStatsRecordEncoderSteps(long x, long y)
{
#if USAGE_STATS_ENABLED
    stats.encoderXSteps += x;
    stats.encoderYSteps += y;
#else
    (void)x; (void)y;
#endif
}

void usageStatsRecordMouseReport(bool usbReport)
{
#if USAGE_STATS_ENABLED
    stats.mouseReports++;
    if (usbReport) stats.usbReports++;
    else stats.bleReports++;
#else
    (void)usbReport;
#endif
}

void usageStatsRecordBleConnect()
{
#if USAGE_STATS_ENABLED
    stats.bleConnects++;
    bleSessionStartMs = millis();
    lastBleEventMs = millis();
    lastBleDisconnectReasonValid = false;
#endif
}

void usageStatsRecordBleDisconnect(uint8_t reason)
{
#if USAGE_STATS_ENABLED
    stats.bleDisconnects++;
    if (bleSessionStartMs != 0) {
        lastBleSessionDurationMs = millis() - bleSessionStartMs;
    }
    bleSessionStartMs = 0;
    lastBleEventMs = millis();
    lastBleDisconnectReason = reason;
    lastBleDisconnectReasonValid = true;
#else
    (void)reason;
#endif
}

void usageStatsRecordOutputModeChange(bool usbMode)
{
#if USAGE_STATS_ENABLED
    stats.outputModeChanges++;
    if (usbMode) {
        bleWasConnectedWhenUsbTookPriority = bleMouseConnected();
        if (bleWasConnectedWhenUsbTookPriority && bleSessionStartMs != 0) {
            lastBleSessionDurationMs = millis() - bleSessionStartMs;
        }
        lastBleEventMs = millis();
    }
    usageStatsSave();
#else
    (void)usbMode;
#endif
}

void usageStatsRecordIdleEntry()
{
#if USAGE_STATS_ENABLED
    stats.idleEntries++;
#endif
}

void usageStatsRecordBattery(float voltage, uint8_t percent, bool charging, bool fullyCharged)
{
#if USAGE_STATS_ENABLED
    if (voltage >= 2.5f && voltage <= 4.5f) {
        stats.lastBatteryV = voltage;
        stats.lastBatteryPercent = percent;
        if (voltage < stats.batteryMinV) stats.batteryMinV = voltage;
        if (voltage > stats.batteryMaxV) stats.batteryMaxV = voltage;
    }

    if (charging && !lastCharging) {
        stats.chargingSessions++;
        lastChargePercent = percent;
    }

    if (charging && percent > lastChargePercent) {
        const uint8_t rise = percent - lastChargePercent;
        stats.chargedPercentTenths += (uint32_t)rise * 10UL;
    }

    if (fullyCharged && !lastFullyCharged) {
        stats.fullChargeEvents++;
        dischargeTracking = true;
        dischargeStartPercent = 100;
        dischargeStartRuntimeMs = stats.runtimeMs;
    }

    // A discharge sample runs from >=95% down to <=20% on battery.
    if (!charging && !fullyCharged && !dischargeTracking && percent >= 95) {
        dischargeTracking = true;
        dischargeStartPercent = percent;
        dischargeStartRuntimeMs = stats.runtimeMs;
    }

    if (!charging && !fullyCharged && dischargeTracking && percent <= 20) {
        const uint8_t consumed = dischargeStartPercent > percent
            ? dischargeStartPercent - percent
            : 0;

        if (consumed >= 60) {
            const uint64_t elapsed = stats.runtimeMs - dischargeStartRuntimeMs;
            const uint64_t normalized = elapsed * 100ULL / consumed;
            stats.latestNormalizedDischargeMs = normalized;
            if (normalized > stats.referenceNormalizedDischargeMs) {
                stats.referenceNormalizedDischargeMs = normalized;
            }
            stats.completedDischargeSamples++;
            usageStatsSave();
        }
        dischargeTracking = false;
    }

    lastCharging = charging;
    lastFullyCharged = fullyCharged;
    lastChargePercent = percent;
#else
    (void)voltage; (void)percent; (void)charging; (void)fullyCharged;
#endif
}


bool usageStatsStorageReady()
{
#if USAGE_STATS_ENABLED
    return storageReady;
#else
    return false;
#endif
}
