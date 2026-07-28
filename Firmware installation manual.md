# Installing the GM-6000 Firmware

> [!IMPORTANT]
> This guide is for the **Genius GM-6000 BLE Mouse Revival** project using the **Seeed Studio XIAO nRF52840 Sense**.
>
> It assumes you are using **Windows 10 or Windows 11**.

This guide explains how to install the latest firmware on your GM-6000 mouse using PlatformIO.

**No programming experience is required.**

> [!NOTE]
> **Tested with**
>
> * Windows 11
> * Visual Studio Code 1.104 or newer
> * PlatformIO IDE 3.x

---

<a name="table-of-contents"></a>

## Table of Contents

* [What You Need](#requirements)
* [Step 1 – Install Visual Studio Code](#install-vscode)
* [Step 2 – Install PlatformIO](#install-platformio)
* [Step 3 – Download the Firmware](#download-firmware)
* [Step 4 – Extract the ZIP File](#extract-firmware)
* [Step 5 – Open the Project](#open-project)
* [Step 6 – Connect the Mouse](#connect-mouse)
* [Step 7 – Build the Firmware](#build-firmware)
* [Step 8 – Upload the Firmware](#upload-firmware)
* [Step 9 – Pair the Mouse](#pair-mouse)
* [Reset Bluetooth Pairings](#reset-pairings)
* [Troubleshooting](#troubleshooting)
* [Updating the Firmware](#update-firmware)

---

<a name="requirements"></a>

## What You Need

Before you begin, make sure you have:

* A Windows 10 or Windows 11 computer
* A USB-C **data** cable
* An internet connection
* The latest GM-6000 firmware ZIP file

> [!NOTE]
> Some USB-C cables only provide power. If the mouse is not detected, try another cable.

[Back to Table of Contents](#table-of-contents)

---

<a name="install-vscode"></a>

## Step 1 – Install Visual Studio Code

PlatformIO runs inside Visual Studio Code.

1. Download [Visual Studio Code](https://code.visualstudio.com/download) from the official website.

2. Run the installer.

3. Accept the default installation settings.

4. Start Visual Studio Code.

> [!IMPORTANT]
> Install **Visual Studio Code**.
>
> Do **not** install the larger application called **Visual Studio**.

[Back to Table of Contents](#table-of-contents)

---

<a name="install-platformio"></a>

## Step 2 – Install PlatformIO

1. Open Visual Studio Code.

2. Click the **Extensions** icon on the left.

3. Search for:

   ```text
   PlatformIO IDE
   ```

4. Select **PlatformIO IDE** by **PlatformIO**.

5. Click **Install**.

6. Wait for the installation to finish.

7. Restart Visual Studio Code if prompted.

See the official [PlatformIO installation guide](https://platformio.org/install/ide?install=vscode).

> [!TIP]
> You do **not** need to install Python or the Arduino IDE separately.

[Back to Table of Contents](#table-of-contents)

---

<a name="download-firmware"></a>

## Step 3 – Download the Firmware

Download the latest firmware ZIP file from the project’s **GitHub Releases** page.

Save the ZIP file somewhere easy to find, such as your **Downloads** folder.

[Back to Table of Contents](#table-of-contents)

---

<a name="extract-firmware"></a>

## Step 4 – Extract the ZIP File

Do **not** open the project directly from the ZIP file.

1. Right-click the downloaded ZIP file.

2. Select **Extract All...**

3. Choose a location, for example:

   ```text
   Documents\GM-6000 Firmware
   ```

4. Click **Extract**.

[Back to Table of Contents](#table-of-contents)

---

<a name="open-project"></a>

## Step 5 – Open the Project

1. Open Visual Studio Code.

2. Select:

   **File → Open Folder...**

3. Browse to the extracted firmware folder.

4. Select the folder containing:

   ```text
   platformio.ini
   ```

5. Click **Select Folder**.

6. If Visual Studio Code asks whether you trust the project, click:

   **Trust the Authors**

> [!TIP]
> If you cannot find `platformio.ini`, you probably opened the wrong folder.

The correct project folder should contain files and folders similar to these:

```text
platformio.ini
src
include
lib
```

[Back to Table of Contents](#table-of-contents)

---

<a name="connect-mouse"></a>

## Step 6 – Connect the Mouse

1. Connect the mouse to your computer using a USB-C **data** cable.

2. Turn the mouse on.

Windows should detect the mouse automatically.

[Back to Table of Contents](#table-of-contents)

---

<a name="build-firmware"></a>

## Step 7 – Build the Firmware

Building the project checks that everything is ready before uploading.

**It does not change anything on the mouse.**

### Method 1 – Project Tasks

1. Click the **PlatformIO** icon, usually shown as an alien-head symbol, in the left sidebar.

2. Expand:

   ```text
   Project Tasks
   ```

3. Expand:

   ```text
   General
   ```

4. Click:

   ```text
   Build
   ```

### Method 2 – Build Button

If a **Build (✓)** button is visible in your PlatformIO interface, you can use that instead.

> [!NOTE]
> The first build may take several minutes.
>
> PlatformIO automatically downloads the compiler, board support package and required libraries during the first build.

When the build finishes successfully, the terminal should end with:

```text
SUCCESS
```

Do **not** continue if the build ends with:

```text
FAILED
```

[Back to Table of Contents](#table-of-contents)

---

<a name="upload-firmware"></a>

## Step 8 – Upload the Firmware

### Method 1 – Project Tasks

1. Open:

   ```text
   Project Tasks
   ```

2. Expand:

   ```text
   General
   ```

3. Click:

   ```text
   Upload
   ```

### Method 2 – Upload Button

If an **Upload (→)** button is visible in your PlatformIO interface, you can use that instead.

PlatformIO will now:

* Build the firmware
* Upload it to the mouse
* Restart the mouse automatically

When the upload is complete, the terminal should end with:

```text
SUCCESS
```

> [!WARNING]
> Do not disconnect the USB cable while the firmware is being uploaded.

[Back to Table of Contents](#table-of-contents)

---

<a name="pair-mouse"></a>

## Step 9 – Pair the Mouse

After uploading:

1. Disconnect the USB cable if you want to use Bluetooth.

2. Turn the mouse off and back on.

3. Open your computer’s Bluetooth settings.

4. Select:

   ```text
   Genius GM-6000 BLE mouse
   ```

5. Complete the pairing process.

Your mouse is now ready to use.

[Back to Table of Contents](#table-of-contents)

---

<a name="reset-pairings"></a>

## Reset Bluetooth Pairings

Use this function if the mouse cannot pair with a new computer or repeatedly connects and disconnects during pairing.

1. Turn the mouse **off**.

2. Press and hold all three mouse buttons:

   * Left mouse button
   * Middle mouse button
   * Right mouse button

3. While holding all three buttons, turn the mouse **on**.

4. Keep holding the buttons for **5 seconds**.

5. Release the buttons.

The mouse will erase all stored Bluetooth pairings and restart automatically.

You can now pair it as a new Bluetooth mouse.

> [!NOTE]
> If your mouse has an external dual-colour status LED installed, the red and blue LEDs will alternate while you hold the buttons.
>
> After the stored pairings have been erased, both colours will flash three times before the mouse restarts.

> [!IMPORTANT]
> You may also need to remove the old mouse entry from your computer’s Bluetooth settings before pairing it again.

[Back to Table of Contents](#table-of-contents)

---

<a name="troubleshooting"></a>

## Troubleshooting

<details>
<summary><strong>The mouse is not detected</strong></summary>

Try the following:

* Make sure you are using a USB-C **data** cable.
* Try another USB port.
* Try another USB cable.
* Disconnect and reconnect the mouse.
* Turn the mouse off and back on.

</details>

<details>
<summary><strong>Enter Bootloader Mode</strong></summary>

If uploading still fails:

1. Leave the mouse connected by USB.

2. Quickly press the **RESET** button on the XIAO **twice**.

3. A new COM port should appear.

4. Try uploading again.

> [!NOTE]
> The COM port number may change when the XIAO enters bootloader mode. This is normal.

See the official [Seeed Studio XIAO nRF52840 documentation](https://wiki.seeedstudio.com/XIAO_BLE/).

</details>

<details>
<summary><strong>PlatformIO reports “Access is denied”</strong></summary>

If you receive an error similar to:

```text
PermissionError: Access is denied
```

Another program is probably already using the serial port.

Close programs such as:

* PlatformIO Serial Monitor
* PuTTY
* Another serial terminal
* Another open instance of Visual Studio Code

Then try uploading again.

</details>

<details>
<summary><strong>PlatformIO uses the wrong COM port</strong></summary>

Normally PlatformIO detects the correct COM port automatically.

If it does not:

1. Open:

   ```text
   platformio.ini
   ```

2. Find or add:

   ```ini
   upload_port =
   ```

3. Set it to the correct COM port, for example:

   ```ini
   upload_port = COM19
   ```

4. Save the file.

5. Try uploading again.

If the COM port changes later, update or remove this line.

> [!NOTE]
> The normal firmware mode and bootloader mode may use different COM port numbers.

</details>

<details>
<summary><strong>The build fails</strong></summary>

If the project does not build:

* Make sure the ZIP file was fully extracted.
* Make sure you opened the folder containing `platformio.ini`.
* Check that your internet connection is working.
* Verify that PlatformIO IDE is installed correctly.
* Restart Visual Studio Code.
* Try building the project again.

If necessary, extract a fresh copy of the firmware ZIP and open the new folder.

</details>

<details>
<summary><strong>The upload succeeds, but Bluetooth pairing fails</strong></summary>

Try the following:

1. Remove the existing **Genius GM-6000 BLE mouse** entry from your computer’s Bluetooth settings.

2. Reset the stored Bluetooth pairings in the mouse by holding the left, middle and right buttons for 5 seconds during startup.

3. Turn the mouse off and back on.

4. Pair the mouse again.

</details>

<details>
<summary><strong>The mouse repeatedly connects and disconnects</strong></summary>

This can be caused by an old or incompatible Bluetooth pairing.

1. Remove the mouse from the computer’s Bluetooth settings.

2. Reset the stored pairings in the mouse.

3. Restart the computer’s Bluetooth adapter if necessary.

4. Pair the mouse again as a new device.

</details>

[Back to Table of Contents](#table-of-contents)

---

<a name="update-firmware"></a>

## Updating the Firmware

To install a newer firmware version:

1. Download the new firmware ZIP file.

2. Extract it to a new folder.

3. Open the new project folder in Visual Studio Code.

4. Build the project.

5. Upload the firmware.

The mouse will restart automatically with the new firmware.

> [!TIP]
> Keep the previous working firmware ZIP file as a backup until you have tested the new version.

[Back to Table of Contents](#table-of-contents)
