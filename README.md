| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-H21 | ESP32-H4 | ESP32-P4 | ESP32-S2 | ESP32-S3 | ESP32-S31 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | --------- | -------- | -------- | -------- | -------- | --------- |

# Blink Example

## This project: Freenove ESP32-S3 WROOM Board Lite (N16R8)

Open `blink.code-workspace` in VS Code, or open this `blink` folder directly.
Do not open only its parent `esp-udp` folder: ESP-IDF expects the project
`CMakeLists.txt` at the workspace root.

The saved configuration targets `esp32s3`, uses the onboard addressable RGB
LED on **GPIO48** with the RMT driver, and toggles dim white/off every
**1,000 ms**. Flash size is **16 MB**. PSRAM is not needed for this example
and is left disabled. The board's separate conventional LED is on GPIO2.

Connect the board's **USB-UART** socket with a USB data cable. On this computer
it is **COM6**; COM4 and COM5 are Bluetooth ports. If Windows assigns another
port, use **ESP-IDF: Select Port to Use**.

In the VS Code Command Palette:

1. **ESP-IDF: Build your project**
2. **ESP-IDF: Flash your project** (UART)
3. **ESP-IDF: Monitor your device** (115200 baud; exit with **Ctrl+]**)

Use the ESP-IDF commands, not the generic CMake Tools Build button or F5.
UART flashing does not require OpenOCD/JTAG.

For a PowerShell alternative using the existing installation:

```powershell
Set-Location 'C:\Users\ryane\source\repos\esp-udp\blink'
. 'C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1'
$env:PYTHONUTF8 = '1'
$env:PYTHONIOENCODING = 'utf-8'
idf.py build
idf.py -p COM6 -b 460800 flash
idf.py -p COM6 monitor
```

The activation script supplies the installed Python, CMake, Ninja, compiler,
and ESP-IDF environment. An ordinary terminal without activation is not enough.
This EIM installation uses `C:\Espressif\tools` as `IDF_TOOLS_PATH`, not
`C:\Espressif`.

This project is intentionally configured only for the Freenove ESP32-S3.
The repository does not require the optional VS Code Dev Container: the
Windows ESP-IDF installation is the working development environment, and the
board's USB-UART port is used directly by Windows. The removed container was
an Espressif QEMU-oriented template and was not used for this hardware.

If flashing cannot connect, close other serial monitors, hold **BOOT**, tap
**RST**, release **BOOT**, and retry. Tap **RST** without holding BOOT to
return to normal execution if needed.

Defaults apply when generating a new `sdkconfig`. To change an existing
configuration, use **ESP-IDF: SDK Configuration Editor (Menuconfig)**:
under **Example Configuration**, select LED strip, RMT, GPIO48, and 1000 ms.

See [the setup report](BLINK-SETUP-REPORT.md) for diagnosis, changes, and
verification results.

(See the README.md file in the upper level 'examples' directory for more information about examples.)

This example demonstrates how to blink a LED by using the GPIO driver or using the [led_strip](https://components.espressif.com/component/espressif/led_strip) library if the LED is addressable e.g. [WS2812](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf). The `led_strip` library is installed via [component manager](main/idf_component.yml).

## How to Use Example

Before project configuration and build, be sure to set the correct chip target using `idf.py set-target <chip_name>`.

### Hardware Required

* A development board with normal LED or addressable LED on-board (e.g., ESP32-S3-DevKitC, ESP32-C6-DevKitC etc.)
* A USB cable for Power supply and programming

See [Development Boards](https://www.espressif.com/en/products/devkits) for more information about it.

### Configure the Project

Open the project configuration menu (`idf.py menuconfig`).

In the `Example Configuration` menu:

* Select the LED type in the `Blink LED type` option.
  * Use `GPIO` for regular LED
  * Use `LED strip` for addressable LED
* If the LED type is `LED strip`, select the backend peripheral
  * `RMT` is only available for ESP targets with RMT peripheral supported
  * `SPI` is available for all ESP targets
* Set the GPIO number used for the signal in the `Blink GPIO number` option.
* Set the blinking period in the `Blink period in ms` option.

### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

As you run the example, you will see the LED blinking, according to the previously defined period. For the addressable LED, you can also change the LED color by setting the `led_strip_set_pixel(led_strip, 0, 16, 16, 16);` (LED Strip, Pixel Number, Red, Green, Blue) with values from 0 to 255 in the [source file](main/blink_example_main.c).

```text
I (315) example: Example configured to blink addressable LED!
I (325) example: Turning the LED OFF!
I (1325) example: Turning the LED ON!
I (2325) example: Turning the LED OFF!
I (3325) example: Turning the LED ON!
I (4325) example: Turning the LED OFF!
I (5325) example: Turning the LED ON!
I (6325) example: Turning the LED OFF!
I (7325) example: Turning the LED ON!
I (8325) example: Turning the LED OFF!
```

Note: The color order could be different according to the LED model.

The pixel number indicates the pixel position in the LED strip. For a single LED, use 0.

## Troubleshooting

* If the LED isn't blinking, check the GPIO or the LED type selection in the `Example Configuration` menu.

For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you soon.
