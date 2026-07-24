#include "BleMouse.h"
#include "Config.h"
#include "UsageStats.h"

BLEDis bledis;
BLEHidAdafruit blehid;
BLEBas blebas;
BLECharacteristic bleBatteryPowerState(0x2A1A);

static bool bleAdvertisingAllowed = true;
static uint16_t activeConnHandle = BLE_CONN_HANDLE_INVALID;
static bool idleProfileRequested = false;
static bool bleInputEnabled = true;

static void requestConnectionProfile(uint16_t interval, uint16_t supervisionTimeout)
{
    if (!Bluefruit.connected() || activeConnHandle == BLE_CONN_HANDLE_INVALID) {
        return;
    }

    BLEConnection *connection = Bluefruit.Connection(activeConnHandle);
    if (connection == nullptr) {
        return;
    }

    connection->requestConnectionParameter(
        interval,
        0,
        supervisionTimeout
    );
}

static void connect_callback(uint16_t conn_handle)
{
    activeConnHandle = conn_handle;
    idleProfileRequested = false;
    requestConnectionProfile(
        BLE_ACTIVE_CONN_INTERVAL,
        BLE_ACTIVE_SUP_TIMEOUT
    );
    usageStatsRecordBleConnect();
}

static void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    (void)conn_handle;
    (void)reason;

    activeConnHandle = BLE_CONN_HANDLE_INVALID;
    idleProfileRequested = false;
    usageStatsRecordBleDisconnect(reason);

    if (bleAdvertisingAllowed) {
        bleMouseStartAdvertising();
    }
}

void bleMouseSetAdvertisingAllowed(bool allowed)
{
    bleAdvertisingAllowed = allowed;

    if (!allowed) {
        Bluefruit.Advertising.stop();
    }
}

void bleMouseStartAdvertising()
{
    if (!bleAdvertisingAllowed) return;
    if (Bluefruit.connected()) return;
    if (Bluefruit.Advertising.isRunning()) return;

    Bluefruit.Advertising.stop();
    Bluefruit.Advertising.clearData();
    Bluefruit.ScanResponse.clearData();

    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_MOUSE);
    Bluefruit.Advertising.addService(blehid);
    Bluefruit.Advertising.addService(blebas);
    Bluefruit.ScanResponse.addName();

    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(160, 800);
    Bluefruit.Advertising.start(0);
}

void bleMouseBegin()
{
    Bluefruit.begin(1, 0);

    // Prevent the Bluefruit core from driving an onboard connection LED.
    // The GM-6000 uses its own external status LEDs, and the onboard LED
    // would otherwise waste power whenever BLE is connected.
    Bluefruit.autoConnLed(false);

    // Preferred range during the initial connection negotiation.
    Bluefruit.setTxPower(BLE_TX_POWER);
    Bluefruit.setName(BLE_DEVICE_NAME);

    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    bledis.setManufacturer("Genius");
    bledis.setModel("GM-6000 BLE Mouse");
    bledis.setFirmwareRev(FIRMWARE_VERSION_STRING);
    bledis.setHardwareRev(FIRMWARE_HARDWARE_REV);
    bledis.begin();

    blebas.begin();

    // Optional standard Battery Power State characteristic (UUID 0x2A1A).
    // It is attached to the Battery Service and can be read by BLE tools.
    bleBatteryPowerState.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
    bleBatteryPowerState.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    bleBatteryPowerState.setFixedLen(1);
    bleBatteryPowerState.begin();
    bleBatteryPowerState.write8(0);

    blehid.begin();

    // Important: place this after blehid.begin(), because the HID service
    // can set its own preferred connection interval during initialization.
    Bluefruit.Periph.setConnInterval(6, 9);

    blebas.write(0);

    if (bleAdvertisingAllowed) {
        bleMouseStartAdvertising();
    }
}

void bleMouseUseActiveProfile()
{
    if (!idleProfileRequested) {
        return;
    }

    idleProfileRequested = false;
    requestConnectionProfile(
        BLE_ACTIVE_CONN_INTERVAL,
        BLE_ACTIVE_SUP_TIMEOUT
    );

#if DEBUG_ENABLED && DEBUG_BLE
    Serial.println("BLE profile requested: ACTIVE");
#endif
}

void bleMouseUseIdleProfile()
{
    if (idleProfileRequested) {
        return;
    }

    idleProfileRequested = true;
    requestConnectionProfile(
        BLE_IDLE_CONN_INTERVAL,
        BLE_IDLE_SUP_TIMEOUT
    );

#if DEBUG_ENABLED && DEBUG_BLE
    Serial.println("BLE profile requested: IDLE");
#endif
}

bool bleMouseConnected()
{
    return Bluefruit.connected();
}

bool bleMouseInputEnabled()
{
    return bleInputEnabled;
}

bool bleMouseIdleProfileRequested()
{
    return idleProfileRequested;
}

void bleMouseSetInputEnabled(bool enabled)
{
    bleInputEnabled = enabled;
}

bool bleMouseTrySendReport(const MouseReportData &report)
{
    if (!bleInputEnabled) return false;
    if (!Bluefruit.connected()) return false;

    return blehid.mouseReport(
        report.buttons,
        report.x,
        report.y,
        report.wheel,
        report.pan
    );
}

void bleMouseBatteryWrite(uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }

    blebas.write(percent);
}

void bleMouseBatteryPowerStateWrite(uint8_t state)
{
    bleBatteryPowerState.write8(state);

    if (Bluefruit.connected()) {
        bleBatteryPowerState.notify8(state);
    }
}

void bleMouseDisconnectIfConnected()
{
    bleAdvertisingAllowed = false;
    Bluefruit.Advertising.stop();

    if (Bluefruit.connected()) {
        Bluefruit.disconnect(0);
        delay(100);
    }
}
