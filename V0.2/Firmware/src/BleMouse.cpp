#include "BleMouse.h"
#include "Config.h"

BLEDis bledis;
BLEHidAdafruit blehid;
BLEBas blebas;

static void connect_callback(uint16_t conn_handle) {
    (void)conn_handle;
}

static void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
    (void)conn_handle;
    (void)reason;
    bleMouseStartAdvertising();
}

void bleMouseStartAdvertising() {
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

void bleMouseBegin() {
    Bluefruit.begin(1, 0);

    Bluefruit.Periph.setConnInterval(6, 12);
    Bluefruit.setTxPower(BLE_TX_POWER);
    Bluefruit.setName(BLE_DEVICE_NAME);

    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    bledis.setManufacturer("Genius");
    bledis.setModel("GM-6000 BLE Mouse");
    bledis.begin();

    blebas.begin();
    blehid.begin();

    // Start met een bekende veilige waarde.
    // De echte batterijwaarde wordt later door Battery.cpp geschreven.
    blebas.write(0);

    bleMouseStartAdvertising();
}

bool bleMouseConnected() {
    return Bluefruit.connected();
}

void bleMouseSendReport(const MouseReportData &report) {
    if (!Bluefruit.connected()) return;

    blehid.mouseReport(
        report.buttons,
        report.x,
        report.y,
        report.wheel,
        report.pan
    );
}

void bleMouseBatteryWrite(uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }

    // Altijd schrijven, ook als er nog geen BLE-client verbonden is.
    // Zo staat de Battery Service alvast goed wanneer Windows verbindt.
    blebas.write(percent);
}

void bleMouseDisconnectIfConnected() {
    Bluefruit.Advertising.stop();

    if (Bluefruit.connected()) {
        Bluefruit.disconnect(0);
        delay(100);
    }
}