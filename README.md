# EdgeLink Gateway

ESP32 gateway firmware built with ESP-IDF. The current application:

- connects to a configured Wi-Fi network;
- generates fake temperature, pressure, voltage, and current readings;
- passes readings through a FreeRTOS queue;
- publishes readings as JSON to an MQTT broker; and
- blinks the onboard LED on GPIO 2.

## Hardware

- DOIT ESP32 DevKit V1 (original ESP32 target)
- Data-capable USB cable
- Linux development computer

## 1. Install ESP-IDF on Debian or Ubuntu

Espressif recommends the ESP-IDF Installation Manager for ESP-IDF 6 and newer.
Install its command-line interface:

```bash
echo "deb [trusted=yes] https://dl.espressif.com/dl/eim/apt/ stable main" \
  | sudo tee /etc/apt/sources.list.d/espressif.list
sudo apt update
sudo apt install eim-cli
```

Install the latest stable ESP-IDF and its development tools:

```bash
eim install
```

The installer prints the activation-script path when it finishes. Activate the
installed environment, for example:

```bash
source ~/.espressif/tools/activate_idf_v6.0.2.sh
```

Use the exact filename produced by your installation. Confirm that ESP-IDF is
available:

```bash
idf.py --version
```

The environment must be activated in each new terminal before running
`idf.py`. Official installation instructions are available in the
[ESP-IDF Linux setup guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started-cmake/linux-setup.html).

## 2. Download the project

Clone the repository and enter its directory:

```bash
git clone <repository-url> edgelink_gateway
cd edgelink_gateway
```

Replace `<repository-url>` with the Git remote URL used for this project.

The first configure or build downloads the MQTT component specified by
`main/idf_component.yml`. The committed `dependencies.lock` selects the tested
dependency version. Do not manually copy the `managed_components` or `build`
directories from another machine.

## 3. Select the ESP32 target

Run this once for a fresh checkout:

```bash
idf.py set-target esp32
```

This creates a machine-local `sdkconfig` file.

## 4. Configure Wi-Fi and MQTT

Open the configuration interface:

```bash
idf.py menuconfig
```

Configure these menus:

```text
EdgeLink Wi-Fi
  Wi-Fi SSID
  Wi-Fi password
  Maximum connection retries

EdgeLink MQTT
  Broker URI
  Publish topic
  Username
  Password
```

Example settings for an anonymous MQTT broker on the local network:

```text
Broker URI: mqtt://192.168.1.100:1883
Topic:      edgelink/sensors
Username:   (empty)
Password:   (empty)
```

Use the broker account's username and password when authentication is enabled.
Save the configuration before exiting.

The generated `sdkconfig` contains credentials in plain text and is intentionally
excluded from Git. Never edit `build/config/sdkconfig.h`; ESP-IDF generates it
from `sdkconfig`.

## 5. Build the firmware

```bash
idf.py build
```

The firmware image is generated under `build/`.

## 6. Connect and flash the ESP32

Connect the board and locate its serial device:

```bash
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

The DOIT board commonly appears as `/dev/ttyUSB0`. Allow your Linux account to
access serial devices if required:

```bash
sudo usermod -aG dialout "$USER"
```

Log out and back in after changing group membership. Flash the firmware and open
the serial monitor:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Replace `/dev/ttyUSB0` if the board uses a different device. Press `Ctrl+]` to
exit the monitor.

## MQTT payload

Sensor readings are published with QoS 1 in this format:

```json
{
  "device_id": 1,
  "type": "temperature",
  "value": 22.4,
  "timestamp_ms": 5000,
  "valid": true
}
```

`timestamp_ms` is elapsed time since the ESP32 started, not Unix time.

## Useful commands

```bash
idf.py menuconfig       # Change project settings
idf.py build            # Build the firmware
idf.py clean            # Remove compiled objects
idf.py fullclean        # Remove the complete build configuration
idf.py flash            # Build and flash using the detected/configured port
idf.py monitor          # Open the serial monitor
```

If CMake reports that `$IDF_PATH/tools/cmake/project.cmake` cannot be found,
activate the ESP-IDF environment in the current terminal and retry.
