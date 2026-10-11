# Home Automation

ESP32 firmware for my MQTT home-automation devices, built with PlatformIO in VS Code.

| Project | Board | What it does |
|---|---|---|
| `GarageDoor_V4` | DFRobot FireBeetle ESP32 (`firebeetle32`) | Garage door opener button and open/closed sensing |
| `StairLights_V5` | ESP32 DOIT DevKit V1 | WS2812 stair lighting (colour, brightness, pattern over MQTT) |
| `SumpMonitor_V5` | ESP32 DOIT DevKit V1 | Sump pit float switches, pump cycle times |

All three share the same MQTT_Minimal framework (`include/MQTT_Minimal.h` and the WiFi/MQTT/setup code in `src/main.cpp`):

- Connects to WiFi and an MQTT broker; reports to OpenHAB over MQTT.
- If WiFi fails for 60 s or MQTT for 2 min, starts a setup access point named after the device's hostname at `192.168.42.1`. Joining it opens the setup page automatically (captive portal).
- The setup page saves WiFi/MQTT/time-zone settings to LittleFS; passwords are never shown, and leaving one blank keeps it.
- Over-the-air updates at `http://<device-ip>/update` (ElegantOTA).

## Building

1. Copy `include/secrets.example.h` to `include/secrets.h` in the project and fill in your WiFi and MQTT settings. `secrets.h` is git-ignored.
2. Open the project folder in VS Code with the PlatformIO extension, or run `pio run -d <project>`; `pio run -d <project> -t upload` to flash.
