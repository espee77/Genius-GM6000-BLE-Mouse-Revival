# GM-6000 BLE Mouse - Modular refactor-only baseline

This PlatformIO project is a modular refactor of the known-good GM-6000 BLE ball mouse firmware.

Current intent:
- Preserve the working ball mouse behavior first.
- Keep the code split into maintainable modules.
- AirMouse files are included as a separate module, but `AIR_MOUSE_ENABLED` is currently set to `false` in `src/Config.h` so the first test should behave like the stable ball mouse firmware.

Main behavior in this baseline:
- BLE HID mouse
- Ball encoder cursor movement
- Left / right / middle mouse buttons
- Middle-button hold + Y movement scroll
- Battery service
- Light sleep: IR encoder LEDs off
- Deep sleep: wake by buttons
- Status LEDs gated by upside-down orientation

Open this folder in VS Code + PlatformIO and build/upload the `xiaoblesense` environment.

After this baseline is confirmed working, enable AirMouse by changing:

```cpp
#define AIR_MOUSE_ENABLED false
```

to:

```cpp
#define AIR_MOUSE_ENABLED true
```

in `src/Config.h`.


## AirMouse enabled build

This build enables `AIR_MOUSE_ENABLED true` in `src/Config.h`.

Mode behavior:

- Normal flat orientation: ball mouse mode.
- Left or right side orientation: AirMouse mode using gyro X/Z mapping from the tested v6c-style code.
- Button handling is central in `main.cpp`: left, right, and middle buttons are read once by `Buttons.cpp`, passed into either `BallMouse` or `AirMouse`, and only one BLE HID `mouseReport()` is sent per loop by `BleMouse.cpp`.
- AirMouse does not send its own BLE reports and does not force `buttons = 0`, so clicks still work in AirMouse mode.

Important: test ball mode, AirMouse mode, buttons, light sleep, deep sleep, and Windows reconnect after flashing.
