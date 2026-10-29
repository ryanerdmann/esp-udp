# ESP32-S3 Blink

ESP-IDF firmware for the **Freenove ESP32-S3 WROOM Board Lite (N16R8)**.
The onboard addressable RGB LED on **GPIO48** alternates dim white and off
every **1,000 ms**, using the RMT driver. The separate GPIO2 LED is not used.
Flash size is **16 MB**; PSRAM is not needed and is left disabled.

## Build, flash, and monitor

Connect the board's **USB-UART** socket with a USB data cable.

### VS Code (GUI)

Open this repository's root folder in VS Code. With ESP-IDF installed and
configured, use the Command Palette:

1. **ESP-IDF: Build your project**
2. **ESP-IDF: Select Port to Use**
3. **ESP-IDF: Flash your project** (UART)
4. **ESP-IDF: Monitor your device** (115200 baud)

Use the ESP-IDF commands, not the generic CMake Tools Build button or F5.

### Command line (PowerShell)

From the repository root, activate the existing Windows ESP-IDF installation,
then build, flash, and monitor:

```powershell
. 'C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1'
$env:PYTHONUTF8 = '1'
$env:PYTHONIOENCODING = 'utf-8'
idf.py build
idf.py -p COM6 -b 460800 flash
idf.py -p COM6 monitor
```

The activation script path and COM6 are machine-specific; substitute your
installation's script and assigned serial port. Exit the monitor with **Ctrl+]**.

## Configuration

- [Firmware](main/main.c): fixed GPIO48/RMT driver, brightness,
  and blink interval. Edit the constants to change the LED behavior.
- [sdkconfig.defaults](sdkconfig.defaults): ESP32-S3 target and 16 MB flash.
  Defaults apply when generating a new `sdkconfig`; use **ESP-IDF: SDK
  Configuration Editor (Menuconfig)** to change an existing configuration.
- [Component manifest](main/idf_component.yml) and
  [lockfile](dependencies.lock): restore the LED-strip dependency automatically.
  Keep both committed.

The LED lifecycle/state APIs must be called serially by an owning task.
`led_del()` requests cooperative shutdown and blocks until the worker has
finished accessing the LED, then releases the semaphore and strip driver.
It returns an `esp_err_t` that callers must check. Do not call it from the LED
worker or initialize an already-initialized LED without deleting it first.

VS Code uses Microsoft C/C++ and the generated `build/compile_commands.json`.
Run `idf.py reconfigure` if that file is missing. Build outputs, `sdkconfig`,
and managed components are generated and ignored by Git.

Keep local installation paths, compiler paths, and serial-port preferences in
VS Code **User Settings**, not repository settings. When setting up another
computer, configure ESP-IDF and select the serial port there. If C/C++ needs a
fallback compiler before the first build, set `C_Cpp.default.compilerPath` in
User Settings to the ESP32-S3 cross-compiler.

## Troubleshooting

- If flashing cannot connect, close other serial monitors, hold **BOOT**, tap
  **RST**, release **BOOT**, and retry.
- Tap **RST** without holding BOOT to return to normal execution.
- If IntelliSense reports missing headers, build or reconfigure first.
- If terminal tools are missing, activate the ESP-IDF environment rather than
  assembling compiler and Python paths manually.
