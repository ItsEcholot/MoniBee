# MoniBee 🐝

A custom ZigBee device designed for adding home automations to your monitor / workstation setup based on the ESP32-C6 MCU.

## Features

- CEC Control via HDMI:
  - Switch monitor input sources
  - Control power state
  - Adjust backlight brightness
- Ambient Light Sensor:
  - [VEML7700](https://www.vishay.com/docs/84286/veml7700.pdf) is used for accurate ambient light measurements
- IR Transmitter (NEC Protocol):
  - Supports sending custom commands over ZigBee for controlling various devices (Speakers, KVM switches, ...)
- ZigBee Communication:
  - [External converter](z2m_external_converter.js) for [Zigbee2Mqtt](https://www.zigbee2mqtt.io)
    - Allows for integration into [HomeAssistant](https://www.home-assistant.io) for easy automation
  - Low power usage (average of 1mah, could be further reduced with less aggressive CEC polling & ambient light measurements)

## ZigBee endpoints & clusters
- Internal temperature (ESP32-C6 sensor), only accurate if chip is hot:
  - Endpoint: `10`
  - Clusters: Basic, identify cluster and temperature measurement (HA profile) as server side. Identify cluster as client side.
- Ambient light (VEML7700):
  - Endpoint: `11`
  - Clusters: Basic, identify and illuminance clusters (HA profile) as server side.
- Monitor DDC:
  - Endpoint: `12`
  - Clusters: Basic, identify and custom cluster for ddc control with the following attributes as server side:
    - `0x0000 R/W, Reporting, 16bit enum`: Input select (see [ddc.h](src/ddc.h) for enum values)
    - `0x0001 R/W, Reporting, 16bit unsigned`: Brightness (in percent)
    - `0x0002 R/W, Reporting, 8bit enum`: Power mode
- IR Emitter:
  - Endpoint: `13`
  - Clusters: Basic, identify and custom cluster for ir commands without attributes but a **custom command** as server side:
    - `0x0000, payload: 4 bytes, bytes 1 & 2: NEC address (little endian), bytes 3 & 4: NEC command (little endian)`: IR command

## Board
The Tenstar Robot ESP32-C6 Supermini ([AliExpress link](https://www.aliexpress.com/item/1005006406538478.html)) was used because of its small size and the inclusion of an RGB LED to display the current system status.

## Pinout
| Pin | Usage |
| --- | ----- |
| 0 | I2C SDA (HDMI CEC [I recommend using a logic level shifter, HDMI uses 5v] & VEML7700) |
| 1 | I2C SCL (HDMI CEC [I recommend using a logic level shifter, HDMI uses 5v] & VEML7700) |
| 6 | IR LED Cathode |
| 7 | IR LED Anode (**use a properly sized resistor!**) |
| 8 (Built in RGB LED) | WS2812 LED to show current ZigBee Status (green = reset successful, red = connecting, off = ready) |
| 9 (Boot button) | Reset / Re-pair ZigBee |
| 15 (Built in LED) | on = awake, off = light sleep |

## 3D Printed Case
To house all the electronics I have designed a simple 3d printable case. The files for which can be found in the project root. See pictures & screenshots below for some impressions.

## Screenshots & Pictures
<img src="https://github.com/user-attachments/assets/b93a72c8-e839-457c-bfa2-e8bb0220f8ed" alt="soldered monibee without case" width=50%/>
<img src="https://github.com/user-attachments/assets/0cd01b02-fcbb-479f-a252-865d49ecfebc" alt="monibee mounted to back of monitor with 3d printed case" width=50%/>
<img src="https://github.com/user-attachments/assets/e44d6f57-6ca0-4c1a-9733-e1dbdb13386c" alt="monibee 3d printed mounting tray" width=75%/>
<img src="https://github.com/user-attachments/assets/991a22b6-4321-4d62-a1a1-15a0645e097a" alt="zigbee2-mqtt screenshot" width=75%/>
<img src="https://github.com/user-attachments/assets/66004747-6c1f-4f4c-ae06-81eb97873e0f" alt="homeassistant screenshot" width=75%/>


