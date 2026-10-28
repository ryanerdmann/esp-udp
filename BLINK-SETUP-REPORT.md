# Freenove ESP32-S3 blink setup report

Date: October 27, 2026.

## Hardware and working configuration

| Item | Value |
| --- | --- |
| Board | Freenove ESP32-S3 WROOM Board Lite, N16R8 |
| Detected chip | ESP32-S3, revision v0.2 |
| Detected flash | 16 MB |
| Connection | USB-UART socket, COM6 |
| USB bridge | USB VID 1A86, PID 55D3 |
| LED | Onboard WS2812 addressable RGB LED, GPIO48 |
| Driver | ESP-IDF led_strip component, RMT backend |
| Behavior | Dim white for one second, off for one second |
| Serial console | UART0, 115200 baud |
| PSRAM | Not enabled; blink does not need it |

Freenove's [official Lite pinout](https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board_Lite/blob/main/ESP32S3_Lite_Pinout.png)
identifies GPIO48 as the WS2812 connection and GPIO2 as a separate conventional
LED. This setup blinks the LED marked "48", not the GPIO2 LED.

## What was wrong

### 1. VS Code was using the parent folder as the project

The actual project is `C:\Users\ryane\source\repos\esp-udp\blink`, but the
extension ran from `C:\Users\ryane\source\repos\esp-udp`. Its logs reported:

```text
CMakeLists.txt not found in project directory C:\Users\ryane\source\repos\esp-udp
Set target failed.
```

Setting CMake Tools' `cmake.sourceDirectory` to the child folder does not
redirect the ESP-IDF extension. They are separate extensions.

Added [blink.code-workspace](blink.code-workspace), which opens the correct
folder. Open this workspace or the `blink` folder directly. Switching only
the chat's working folder did not fix the VS Code window; opening the saved
workspace was requested separately.

### 2. The selected serial port was Bluetooth

The parent-folder settings selected COM4, but COM4 and COM5 are Bluetooth
serial ports. The old autodetection logs also tried unavailable Qualcomm UART
ports and reported timeouts/access errors.

After you reconnected the board and used BOOT/RST, Windows exposed the board
on COM6. A targeted esptool probe identified the ESP32-S3 and its 16 MB flash.
The project now explicitly uses COM6 and UART flashing.

The old logs also showed `This chip is ESP32-S3, not ESP32`: the failed target
selection had left a probe using the wrong chip type. The saved project
defaults now specify `esp32s3`.

### 3. The example's LED pin was for a different development board

The original ESP32-S3 defaults selected an addressable LED on GPIO38.
Freenove's Lite board uses GPIO48. Updated the ESP32-S3 defaults to GPIO48,
RMT, a 1000 ms interval, and 16 MB flash.

### 4. Editor configuration was incomplete

The C/C++ configuration had an empty compiler path and forced an x86
IntelliSense mode. It now points to the installed ESP32-S3 GCC compiler and
the real `build/compile_commands.json`. The forced x86 mode was removed.
The compile database supplies the actual include paths, defines, and
per-source compiler options; these should not be recreated manually.

Generic CMake automatic configuration was already disabled in the project
settings and was left disabled. Use ESP-IDF's commands for this project.

### 5. Terminal environment matters

The ESP-IDF installation itself was usable; no reinstall or global Windows
locale change was needed.

During diagnosis, a manually assembled environment incorrectly used
`C:\Espressif` as `IDF_TOOLS_PATH`. That caused a missing-constraints-file
error and selected the system CMake instead of the installed IDF CMake.
Using the existing EIM activation script fixed that diagnostic invocation:

```powershell
. 'C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1'
```

Its correct `IDF_TOOLS_PATH` is `C:\Espressif\tools`, and the selected CMake
is version 4.0.3. The installation includes ESP-IDF 6.1 and Python 3.11.15.

The extension logs also warned about Unicode handling. Added `PYTHONUTF8=1`
and `PYTHONIOENCODING=utf-8` to this project's ESP-IDF extra environment
variables rather than changing the machine-wide locale.

## Changes made

| File | Change |
| --- | --- |
| [sdkconfig.defaults](sdkconfig.defaults) | Default target is ESP32-S3 |
| [sdkconfig.defaults.esp32s3](sdkconfig.defaults.esp32s3) | Freenove GPIO48 RGB LED, RMT, 1000 ms, 16 MB flash |
| [.vscode/settings.json](.vscode/settings.json) | COM6, UART, explicit project build directory, UTF-8 environment |
| [.vscode/c_cpp_properties.json](.vscode/c_cpp_properties.json) | Actual cross-compiler and compile database; removed forced x86 mode |
| [main/blink_example_main.c](main/blink_example_main.c) | Check LED/GPIO API errors, log pin/interval, use pdMS_TO_TICKS |
| [blink.code-workspace](blink.code-workspace) | Reopenable workspace with the correct project root |
| [README.md](README.md) | Board-specific instructions and troubleshooting |

The Espressif example's overall behavior and selectable GPIO/RMT/SPI
implementations were retained. No Arduino installation was required.
The generated `sdkconfig` was created during this work and regenerated after
updating the defaults. Build outputs and installed managed components are
generated files, not hand-maintained source.

## Why optional files were removed

This repository is now intentionally single-target and single-workflow:

- The other `sdkconfig.defaults.*` files supported unrelated Espressif
  targets. Only `sdkconfig.defaults` and `sdkconfig.defaults.esp32s3` are
  relevant to this board.
- `pytest_blink.py` was an Espressif example test requiring the embedded-test
  framework; it is not needed to build, flash, or monitor this application.
- `.clangd` and the GDB `launch.json` were unused by the verified VS Code
  build/flash/monitor workflow. C/C++ uses the generated compile database.
- `.devcontainer` was removed. It was an optional Docker environment named
  `ESP-IDF QEMU`, not required for this Windows installation or USB-UART
  access. Use it only if you later choose a Docker-based Linux workflow.
- Generated `build` and `managed_components` contents are disposable. The
  tracked `main/idf_component.yml` and `dependencies.lock` remain so the
  component manager can restore `espressif/led_strip` deterministically.

## Verification

### Build

Built successfully using the existing EIM environment, with no explicit
target argument on the final build. This confirms the saved defaults select
`esp32s3`.

Confirmed the generated configuration contains:

```text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_BLINK_LED_STRIP=y
CONFIG_BLINK_LED_STRIP_BACKEND_RMT=y
CONFIG_BLINK_GPIO=48
CONFIG_BLINK_PERIOD=1000
CONFIG_ESPTOOLPY_FLASHSIZE="16MB"
```

The application binary was 188,160 bytes, leaving about 82% of the default
application partition free. Configuring 16 MB physical flash does not
automatically enlarge the default application partition.

### Flash

Flashed on COM6 at 460800 baud. Bootloader, partition table, and application
all reported `Hash of data verified`. The board was reset through RTS.
No full-chip erase or eFuse changes were performed.

### Running firmware

Captured 12 seconds of serial output at 115200 baud:

```text
I (58264) example: Turning the LED OFF!
I (59264) example: Turning the LED ON!
I (60264) example: Turning the LED OFF!
I (61264) example: Turning the LED ON!
```

Verified 12 alternating entries and 11 intervals, each exactly 1000 ms.
No panic, backtrace, assertion failure, or ESP_ERROR_CHECK failure appeared
in that capture. You also confirmed that the physical LED marked 48 blinks.

Opening a serial port can toggle DTR/RTS and hold/reset this board. For the
bounded verification capture, both signals were disabled before opening the
port. The port was closed afterwards; no background monitor was left running.

### VS Code extension

An extension Build attempted before opening the correct VS Code workspace
still failed. The parent-folder project-root issue was confirmed in the logs.
You subsequently confirmed opening `blink.code-workspace` / the `blink` folder.
The next automated extension verification was interrupted when the VS Code
tool client disconnected during the workspace transition. You then tested
the commands directly in the corrected workspace and confirmed:

- **ESP-IDF Build:** succeeded, no error.
- **ESP-IDF Flash (UART):** succeeded; the LED is blinking.
- **ESP-IDF Monitor:** shows alternating OFF/ON output at 1000 ms intervals,
  including timestamps 16264, 17264, 18264, 19264, 20264, 21264, and 22264.

The VS Code extension workflow is therefore verified by your direct tests,
in addition to the automated command-line, flash-hash, and serial checks.

## Daily workflow

1. Open [blink.code-workspace](blink.code-workspace).
2. Connect USB-UART. Select COM6, or the board's current port if Windows
   assigns a different number.
3. Run **ESP-IDF: Build your project**.
4. Run **ESP-IDF: Flash your project**, using UART.
5. Run **ESP-IDF: Monitor your device**. Exit with **Ctrl+]** before another
   program needs the serial port.

Do not use F5 for ordinary flashing: F5 starts a debugger. The builtin
OpenOCD configuration concerns debugging through native USB/JTAG, not
flashing through the USB-UART connector.

If connection fails, close other monitors, check the selected port and USB
data cable, then hold BOOT, tap RST, release BOOT, and retry. A normal reset
without BOOT runs the saved application.

To change the interval or pin, use ESP-IDF's SDK Configuration Editor under
**Example Configuration**. Defaults only initialize a new `sdkconfig`;
editing defaults alone does not override existing generated settings.
