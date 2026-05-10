**Restoring a Classic: Genius GM-6000 Wireless BLE Conversion**

This project is an open-source hardware and software conversion for the classic Genius GM-6000 ball mouse.

<img width="1258" height="671" alt="Image" src="https://github.com/user-attachments/assets/43abed16-218f-45bf-b2d3-e24f8a19aeda" />

The goal of this project is simple:

Preserve the original look, mechanics, and feel of a vintage ball mouse while replacing the obsolete electronics with a modern wireless Bluetooth Low Energy (BLE) system.

Instead of installing a modern optical sensor, this project keeps the original mechanical ball encoder system intact. The original encoder wheels are reused and read by a newly designed infrared optical detection system based on IR LEDs and phototransistors. A custom PCB built around the Seeed Studio XIAO nRF52840 Sense provides wireless BLE HID mouse functionality, LiPo battery support, and modern connectivity.

**Project Goals**
Preserve the original external appearance of the mouse
Reuse the original ball and encoder wheel mechanics
Replace the original PCB with a modern custom design
Add wireless BLE connectivity
Integrate rechargeable LiPo battery support
Keep the design simple, repairable, and modifiable
Fully open-source the hardware and firmware

**Features**
Custom KiCad PCB design
Bluetooth Low Energy (BLE) mouse functionality
Rechargeable LiPo battery support
IR optical quadrature encoder system
Three user buttons
Dedicated BLE pairing button
Dual-color status LED
Compact internal layout designed to fit inside the original shell
Open hardware and open firmware approach
Repository Contents

**This repository will contain:**

- Complete KiCad project files
- PCB schematics
- PCB layout files
- Bill of Materials (BOM)
- 3D CAD files of the mouse and PCB assembly
- Firmware source code
- Build notes and assembly documentation
- Photos and test results

**Why Open Source?**

This project was created mainly for fun, learning, and preservation of vintage hardware.
My own electronics, hardware, and software skills are limited, so I decided to make the entire project public so others can:

- improve the design
- optimize the electronics
- improve the firmware
- create alternative PCB revisions
- adapt the project for other vintage mice
- experiment with different sensors or features

If this project helps someone learn something, restore old hardware, or create an even better version, then it has succeeded.

_Contributions, suggestions, forks, and improvements are always welcome._

**Current Status**
PCB designed
Prototype PCB ordered
Mechanical integration in progress
Firmware development ongoing

**Disclaimer**

This is an experimental hobby project.
Use the provided files and information at your own risk.

LiPo batteries and electronic modifications should always be handled carefully and responsibly.

**Acknowledgements**

Special thanks to the retro hardware, maker, and open-source communities whose shared knowledge and projects made this build possible.
