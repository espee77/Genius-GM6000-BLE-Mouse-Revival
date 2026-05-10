**Restoring a Classic: Genius GM-6000 Wireless BLE Conversion**

This project is an open-source hardware and software conversion for the classic Genius GM-6000 ball mouse.

<img width="800" alt="Genius-GM6000" src="https://github.com/user-attachments/assets/d0f44eeb-6676-48ad-923f-00c529bbc1b4" />

**The goal of this project is simple**

Preserve the original look, mechanics, and feel of a vintage ball mouse while replacing the obsolete electronics with a modern wireless Bluetooth Low Energy (BLE) system. Instead of installing a modern optical sensor, this project keeps the original mechanical ball encoder system intact. The original encoder wheels are reused and read by a newly designed infrared optical detection system based on IR LEDs and phototransistors. A custom PCB built around the *Seeed Studio XIAO nRF52840 Sense* provides wireless BLE HID mouse functionality, LiPo battery support, and modern connectivity.

<img width="800" alt="Genius-GM6000 conversion" src="https://github.com/user-attachments/assets/e20515aa-5458-4d16-acf4-d4d742eae415" />

**Project Goals**

- Preserve the original external appearance of the mouse
- Reuse the original ball and encoder wheel mechanics
- Replace the original PCB with a modern custom design
- Add wireless BLE connectivity
- Integrate rechargeable LiPo battery support
- Fully open-source the hardware and firmware

**Features**

- Bluetooth Low Energy (BLE) mouse functionality
- Rechargeable LiPo battery support
- IR optical quadrature encoder system
- Three buttons with original feel & sound
- Dedicated BLE pairing button (on bottom)
- Dual-color status LED (on bottom)
- Compact internal layout designed to fit inside the original shell
- Open hardware and open firmware approach


**This repository will contain**

- 3D CAD files of the GM6000 mouse and PCB assembly
- Complete KiCad 10  project files
- PCB schematics
- PCB layout files
- Firmware source code *
- Build notes and assembly documentation *
- Photos and test results *

(* = still to be made)


**Why Open Source?**

This project was created mainly for fun, learning, and preservation of vintage hardware.
My own electronics, hardware, and software skills are (very) limited, so I decided to make the entire project public so others can:

- improve the design
- optimize the electronics
- improve the firmware
- create alternative PCB revisions

If this project helps someone learn something, restore old hardware, or create an even better version, then it has succeeded.

_Contributions,suggestions, forks, and improvements are always welcome._


**Current Status**

- PCB designed
- Prototype PCB ordered
- Firmware development to be started

**Disclaimer**

This is an experimental hobby project.
Use the provided files and information at your own risk.

LiPo batteries and electronic modifications should always be handled carefully and responsibly.

**Acknowledgements**

Special thanks to the retro hardware, maker, and open-source communities whose shared knowledge and projects made this build possible.
