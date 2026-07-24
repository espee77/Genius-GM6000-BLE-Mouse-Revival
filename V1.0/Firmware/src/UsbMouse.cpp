#include "UsbMouse.h"
#include "Config.h"
#include "Adafruit_TinyUSB.h"

// USB HID mouse using TinyUSB.
// Important behavior:
// - USB charger / powerbank: TinyUSBDevice.mounted() stays false -> BLE remains active.
// - Computer USB host: mounted becomes true -> mouse reports are routed to USB.

static uint8_t const hidReportDescriptor[] = {
    TUD_HID_REPORT_DESC_MOUSE()
};

static Adafruit_USBD_HID usbHid(
    hidReportDescriptor,
    sizeof(hidReportDescriptor),
    HID_ITF_PROTOCOL_MOUSE,
    2,
    false
);

static bool lastMounted = false;
static bool modeChanged = false;

void usbMouseBegin()
{
    // Ontkoppel tijdelijk van de USB-host terwijl descriptors
    // en de HID-interface worden voorbereid.
    TinyUSBDevice.detach();
    delay(10);

    // Deze instellingen moeten vóór usbHid.begin() staan.
    TinyUSBDevice.setManufacturerDescriptor(USB_MANUFACTURER_NAME);
    TinyUSBDevice.setProductDescriptor(USB_PRODUCT_NAME);

    usbHid.setPollInterval(2);
    usbHid.begin();

    // Laat Windows het apparaat opnieuw enumereren,
    // nu met de HID-interface en de juiste naam.
    delay(10);
    TinyUSBDevice.attach();

#if DEBUG_ENABLED && DEBUG_USB
    Serial.print("USB mouse HID started as: ");
    Serial.println(USB_PRODUCT_NAME);
#endif
}

void usbMouseUpdate()
{
    bool mounted = TinyUSBDevice.mounted();

    if (mounted != lastMounted) {
        lastMounted = mounted;
        modeChanged = true;

#if DEBUG_ENABLED && DEBUG_USB
        Serial.print("USB host mounted: ");
        Serial.println(mounted ? "YES - USB mouse mode" : "NO - BLE mouse mode");
#endif
    }
}

bool usbMouseHostMounted()
{
    return TinyUSBDevice.mounted();
}

bool usbMouseModeChanged()
{
    if (!modeChanged) return false;
    modeChanged = false;
    return true;
}

bool usbMouseTrySendReport(const MouseReportData &report)
{
    if (!TinyUSBDevice.mounted()) return false;
    if (!usbHid.ready()) return false;

    return usbHid.mouseReport(
        0,
        report.buttons,
        report.x,
        report.y,
        report.wheel,
        report.pan
    );
}
