# ESP32-S3 Blink

ESP-IDF firmware for the **Freenove ESP32-S3 WROOM Board Lite (N16R8)**.
The onboard addressable RGB LED on **GPIO48** blinks dim white while connecting
to Wi-Fi, then stays solid green after obtaining an IP address, using the RMT driver.
Once connected, an IPv4 UDP server replies `pong` to messages on port **3333**.
The separate GPIO2 LED is not used.
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

## Test UDP ping/pong

After flashing, watch the serial monitor for `Connected, IP address: ...` and
`UDP ping/pong server listening on port 3333`. On a computer on the same LAN,
run this in PowerShell, replacing the example IP with the board's address:

```powershell
$udp = [System.Net.Sockets.UdpClient]::new()
try {
    $udp.Client.ReceiveTimeout = 3000
    $udp.Connect('192.168.1.123', 3333)
    $message = [System.Text.Encoding]::UTF8.GetBytes('ping')
    [void]$udp.Send($message, $message.Length)
    $sender = [System.Net.IPEndPoint]::new([System.Net.IPAddress]::Any, 0)
    $reply = $udp.Receive([ref]$sender)
    [System.Text.Encoding]::UTF8.GetString($reply)
}
finally {
    $udp.Dispose()
}
```

Expected output: `pong`. Each datagram (including an empty one) receives exactly
four ASCII bytes, with no newline or terminating NUL, sent to its source IP and
port. Message content is ignored; the receive buffer retains at most 128 bytes.
The server runs in the application task after Wi-Fi connects and the LED turns green.
Change `UDP_PORT` in [the UDP server](main/udp_serv.c) to use another port.

A receive timeout means no reply arrived within three seconds. Check the IP,
Wi-Fi connection, firewall rules, and guest-network/client isolation settings.
UDP does not guarantee delivery. This is an unauthenticated LAN test service;
do not forward its port to the Internet.

## Configuration

### Local Wi-Fi credentials

1. Open **ESP-IDF: SDK Configuration Editor (Menuconfig)** (or `idf.py menuconfig`).
2. Under **Application Wi-Fi**, enter the network name (SSID) and password.
3. Save, rebuild, and flash. Use a **2.4 GHz** network; ESP32-S3 does not support
   5 GHz Wi-Fi. Password-protected networks must support WPA2 or WPA3.

These settings are saved in the Git-ignored `sdkconfig`, not in tracked source.
Do not copy them into `sdkconfig.defaults`, commit `sdkconfig` with `git add -f`,
or export a defaults file containing credentials. The configuration editor does
not mask the password; avoid sharing its screenshots or configuration dumps.
An empty SSID causes startup to fail with an explicit error. An empty password
selects an open network.

**This prevents accidental source-control exposure, not device extraction.**
Credentials remain in plaintext in local configuration, generated build files,
and the firmware image. Do not share those artifacts. Production devices should
use runtime provisioning and appropriate NVS/flash encryption instead.

[Network API](main/net.h): call `net_init()` once, then
`net_connect(const char *ssid, const char *password)` from one owning task.
Initialization errors are fatal; reboot rather than retrying partial initialization.
The connection call waits up to 30 seconds for DHCP, returning an error on
timeout or after five retries. Failed attempts stop Wi-Fi and may be retried.
After success, the event handler retries later disconnections up to five times,
logging an error if exhausted; it does not block the application. The LED
indicates initial connection success, not ongoing connectivity.
Credentials are not logged or saved to Wi-Fi NVS storage.

- [Application](main/main.c): initializes the LED and selects its state.
- [UDP server](main/udp_serv.c) and [public API](main/udp_serv.h):
  `udp_server_run()` blocks serving ping/pong after Wi-Fi connects.
- [LED wrapper](main/led.c) and [public API](main/led.h): fixed GPIO48/RMT driver, brightness,
  and blink interval. Edit the constants to change the LED behavior.
- [sdkconfig.defaults](sdkconfig.defaults): ESP32-S3 target and 16 MB flash.
  Defaults apply when generating a new `sdkconfig`; use **ESP-IDF: SDK
  Configuration Editor (Menuconfig)** to change an existing configuration.
- [Component manifest](main/idf_component.yml) and
  [lockfile](dependencies.lock): restore the LED-strip dependency automatically.
  Keep both committed.

The LED lifecycle/state APIs must be called serially by an owning task.
Available states are `LED_OFF`, `LED_ON_WHITE`, `LED_ON_BLUE`, `LED_ON_GREEN`,
`LED_ON_RED`, and `LED_BLINK_WHITE`. Solid colors use brightness 16 (out of 255)
per active RGB channel; blinking white toggles every 1,000 ms.
`LED_ON` and `LED_BLINK` remain aliases for the white states.
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
