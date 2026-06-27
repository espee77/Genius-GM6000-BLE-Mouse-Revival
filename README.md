# Restoring a Classic: Genius GM-6000 Wireless BLE / Airmouse conversion

This project is an open-source hardware and firmware conversion for the classic [Genius GM-6000](http://www.tcocd.de/Pictures/Peripheral/Genius/Genius.shtml) mechanical ball mouse.


<img width="800" alt="Genius-GM6000" src="https://github.com/user-attachments/assets/d0f44eeb-6676-48ad-923f-00c529bbc1b4" />

## The Goal

The goal of this project is to preserve the look, feel, and mechanics of the original Genius GM-6000 while replacing its obsolete electronics with a modern Bluetooth Low Energy (BLE) system and add Airmouse functionality.

Instead of replacing the original ball mechanism with a modern optical sensor, this project preserves the original encoder wheels and tracking mechanics. A custom infrared optical detection system using IR LEDs and phototransistors reads the original quadrature encoders, maintaining the unique feel and behavior of the original mouse.

At the heart of the conversion is a custom PCB based on the **Seeed Studio XIAO nRF52840 Sense**, providing Bluetooth LE connectivity, rechargeable Li-Po battery support, intelligent power management, and modern firmware—all while fitting completely inside the original mouse shell.

<img width="800" alt="Genius-GM6000 conversion" src="https://github.com/user-attachments/assets/e20515aa-5458-4d16-acf4-d4d742eae415" />

If this project inspires someone to restore vintage hardware, learn electronics, or build an even better version, then it has already achieved its goal. Contributions, suggestions, forks, and improvements are always welcome.


# Project Goals

* Preserve the original appearance of the mouse
* Preserve the original ball and encoder mechanics
* Replace the original electronics with a modern custom PCB
* Add wireless Bluetooth Low Energy (BLE) connectivity
* Integrate a rechargeable Li-Po battery
* Keep both hardware and firmware fully open source


# Features

## Mouse

* Bluetooth Low Energy (BLE) HID mouse
* Original mechanical ball tracking
* Infrared optical quadrature encoder system
* Adjustable cursor speed
* Three original mouse buttons
* Hold middle button for smooth scrolling

## Air Mouse

* Automatic Air Mouse mode
* Automatic switching based on mouse orientation
* Motion smoothing, tremor reduction and spike filtering
* Full button support while in Air Mouse mode

## Bluetooth

* Compatible with Windows, macOS, Linux, Android and iPadOS
* Automatic BLE reconnection
* Bluetooth Battery Service (battery level reporting)

## Power Management

* Rechargeable Li-Po battery
* Automatic IR LED power control
* Light sleep and deep sleep modes
* Automatic wake-up from movement, IMU motion or button press

## Hardware

* Custom PCB designed specifically for the GM-6000
* Designed around the Seeed Studio XIAO nRF52840 Sense
* Preserves the original buttons and tracking hardware
* Compact design that fits completely inside the original enclosure

---

# Repository Contents

* KiCad 10 project files
* Schematics
* PCB layout
* Firmware source code
* 3D CAD files


---

# Why Open Source?

This project started as a fun way to combine retro hardware with modern electronics while preserving a classic piece of computer history.

By making both the hardware and firmware open source, I hope others can learn from it, improve the design, or use it as the foundation for their own retro hardware projects.

Contributions are always welcome, whether they involve hardware improvements, firmware development, documentation, testing, or entirely new ideas.

---

# Current Status

* Second PCB revision completed
* Stable BLE HID firmware
* Ball Mouse mode fully operational
* Air Mouse mode implemented
* Battery monitoring implemented
* Automatic sleep and power management
* Prototype extensively tested
* Bluetooth signal strength is still being optimized

---

# Disclaimers

* The Genius GM-6 differs slightly from the GM-6000 (encoder positions, contact roller size and rear switch). Support for the GM-6 may be added in the future.
* This is an experimental hobby project.
* Use the provided files and information at your own risk.
* Li-Po batteries and electronic modifications should always be handled carefully and responsibly.

---

# Acknowledgements

Special thanks to the retro hardware, maker, and open-source communities whose shared knowledge, documentation and projects made this conversion possible.
