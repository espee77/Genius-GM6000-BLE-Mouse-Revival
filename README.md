# Restoring a Classic: Genius GM-6000 Wireless BLE / Wired USB / Airmouse conversion

This project is an open-source hardware and firmware conversion for the classic Genius GM-6000 mechanical ball mouse.

<img width="100%" src="https://github.com/user-attachments/assets/e5c73823-e07e-4519-96ad-092ace81aa6d" />

## History

Introduced in the late 1980s, the Genius GM-6000 was one of the flagship serial mice from [KYE Systems](https://www.geniusnet.com/en) under its Genius brand. At a time when the computer mouse was becoming an essential PC peripheral, the [Genius GM-6000](http://www.tcocd.de/Pictures/Peripheral/Genius/Genius.shtml) stood out with its robust optomechanical design, dynamic resolution technology, and support for both Microsoft and Mouse Systems serial protocols. Bundled with graphics software including Menu Maker and Dr. Halo/Dr. Genius, it was marketed as a premium mouse for both business and graphics applications.

By 1989, Genius advertisements described the company as "the largest selling mouse in Europe," with the GM-6000 featured prominently in its international marketing campaigns. Contemporary reviews also praised the mouse for its accuracy, compatibility, and comprehensive software bundle.

More than three decades later, the GM-6000 is still appreciated by collectors and retro-computing enthusiasts for its solid construction and durable mechanical tracking system. This project preserves those original mechanics while replacing the obsolete electronics with modern Bluetooth Low Energy technology, a rechargeable battery, and optional air-mouse functionality.

## The Goal

The goal of this project is to preserve the look, feel, and mechanics of the original Genius GM-6000 while replacing its obsolete electronics with a modern Bluetooth Low Energy (BLE) system and add Airmouse functionality.

<img width="100%" alt="Genius-GM6000" src="https://github.com/user-attachments/assets/d0f44eeb-6676-48ad-923f-00c529bbc1b4" />

Instead of replacing the original ball mechanism with a modern optical sensor, this project preserves the original encoder wheels and tracking mechanics. A custom infrared optical detection system using IR LEDs and phototransistors reads the original quadrature encoders, maintaining the unique feel and behavior of the original mouse.

At the heart of the conversion is a custom PCB based on the **Seeed Studio XIAO nRF52840 Sense**, providing Bluetooth LE connectivity, rechargeable Li-Po battery support, intelligent power management, and modern firmware—all while fitting completely inside the original mouse shell.

<img width="100%" alt="Genius-GM6000 conversion" src="https://github.com/user-attachments/assets/e20515aa-5458-4d16-acf4-d4d742eae415" />

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

* Bluetooth Low Energy (BLE) HID & wired USB mouse
* Original mechanical ball tracking
* Infrared optical quadrature encoder system
* Adjustable cursor speed
* Three original mouse buttons
* Hold middle button for smooth scrolling

## Air Mouse

* Automatic Air Mouse mode (both in BLE or USB mode)
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
* Light sleep and deep sleep modes to get maximum battery life
* Automatic wake-up from movement, IMU motion (after light sleep) or button press (after deep sleep)

## Hardware

* Custom PCB designed specifically for the GM-6000
* Designed around the Seeed Studio XIAO nRF52840 Sense
* Preserves the original buttons and tracking hardware
* Compact design that fits completely inside the original enclosure

---

# Repository Contents

* [KiCad 10 project files](https://github.com/espee77/Genius-GM6000-BLE-Mouse-Revival/tree/main/V1.0/KICAD)
* [Schematics](https://github.com/espee77/Genius-GM6000-BLE-Mouse-Revival/tree/main/V1.0/Schematic)
* [Datasheets](https://github.com/espee77/Genius-GM6000-BLE-Mouse-Revival/tree/main/V1.0/Datasheets)
* [Firmware source code](https://github.com/espee77/Genius-GM6000-BLE-Mouse-Revival/tree/main/V1.0/Firmware)
* [JLCPCB zip file for board ordering](https://github.com/espee77/Genius-GM6000-BLE-Mouse-Revival/tree/main/V1.0/JLCPCB)
* [3D CAD files](https://github.com/espee77/Genius-GM6000-BLE-Mouse-Revival/tree/main/V1.0/3D%20files/Step214)
* [Pictures with assembly notes](https://github.com/espee77/Genius-GM6000-BLE-Mouse-Revival/tree/main/V1.0/Pictures%20with%20assembly%20notes)


---

# Why Open Source?

This project started as a fun way to combine retro hardware with modern electronics while preserving a classic piece of computer history.

By making both the hardware and firmware open source, I hope others can learn from it, improve the design, or use it as the foundation for their own retro hardware projects.

Contributions are always welcome, whether they involve hardware improvements, firmware development, documentation, testing, or entirely new ideas.

---

# Current Status

* [TESTED AND WORKING GREAT -> V1.0 RELEASED!](https://github.com/espee77/Genius-GM6000-BLE-Mouse-Revival/tree/main/V1.0)
* Second PCB revision completed
* Stable BLE HID firmware
* Ball Mouse mode fully operational
* Air Mouse mode implemented
* Battery monitoring implemented
* Automatic sleep and power management
* Prototype extensively tested
* Bluetooth signal is optimized

---

# Disclaimers

* This is an experimental hobby project.
* Use the provided files and information at your own risk.
* Li-Po batteries and electronic modifications can be hazardous if handled improperly. Always follow appropriate safety precautions and use this project at your own risk.
* The Genius GM-6 differs slightly from the GM-6000 (encoder positions, contact roller size and rear switch). 
---

# Acknowledgements

Special thanks to the retro hardware, maker, and open-source communities whose shared knowledge, documentation and projects made this conversion possible.

<img width="100%" src="https://github.com/user-attachments/assets/5d203b25-8db4-4447-b0a6-a5922fd95e02" />
