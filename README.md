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

## MQTT broker: Raspberry Pi (openHABian)

The broker is Mosquitto on the openHABian Raspberry Pi, which also runs OpenHAB.

| Setting | Value |
|---|---|
| Broker address | `192.168.4.45`, port `1883` |
| Pi network interface | `wlan0` (Wi-Fi), static IP managed by NetworkManager |
| Home network (eero) | `192.168.4.0/22`, i.e. `192.168.4.0` to `192.168.7.255`, mask `255.255.252.0` |
| Gateway / DNS | `192.168.4.1` |

MQTT user and password are in each project's `include/secrets.h` (not committed). They can also be changed on a device's setup page.

### The Pi's static IP must use /22, not /24

eero hands out addresses across the whole `192.168.4.0/22` range, so devices can get `192.168.5.x`, `6.x` or `7.x` addresses. If the Pi is set to `192.168.4.45/24`, it treats only `192.168.4.x` as local. It never replies to anything on 5.x–7.x, so those devices can't reach the broker or the OpenHAB UI.

The symptoms:
- Some devices can reach `192.168.4.45` and others can't. The ones that work all have `192.168.4.x` addresses.
- An ESP32 controller's serial output shows `[MQTT] ... state -2 no TCP conn` even though the server, port, user and password are correct.

**Check the Pi's address and mask:**
```bash
ip -4 addr show wlan0
```
It must show `inet 192.168.4.45/22 brd 192.168.7.255`. `/24` is wrong.

**Fix it (NetworkManager).** Find the connection name for `wlan0`. It's usually the SSID or `preconfigured`:
```bash
nmcli -f NAME,DEVICE con show --active
```
Then set the /22 mask and re-apply it as one command, so it completes even if SSH drops while Wi-Fi reconnects:
```bash
sudo nmcli con mod "<name>" ipv4.addresses 192.168.4.45/22 ipv4.gateway 192.168.4.1 && sudo nmcli con up "<name>"
```
SSH freezes for a few seconds while Wi-Fi reconnects. If it doesn't come back within a minute, power-cycle the Pi; the setting is saved.

On older images that use dhcpcd instead, edit `/etc/dhcpcd.conf`, change `static ip_address=192.168.4.45/24` to `/22`, and reboot.

**It persists across reboots.** `nmcli con mod` writes the connection profile to `/etc/NetworkManager/system-connections/<name>.nmconnection`. To check it:
```bash
sudo grep -A4 "^\[ipv4\]" /etc/NetworkManager/system-connections/*.nmconnection
```
It should show `address1=192.168.4.45/22,192.168.4.1`. Re-running the network setup in `openhabian-config` can rewrite this profile, so check the mask again afterwards.

**An alternative to a static IP:** set the Pi to DHCP and add an IP reservation for `192.168.4.45` in the eero app. The mask then always matches what eero hands out.

### Moving to a different router or Pi address

The values above (`192.168.4.45`, `/22`, gateway `192.168.4.1`) only fit this eero network. On any router, a static IP on the Pi must match three things:

| Setting | Must be |
|---|---|
| IP address | Inside the router's subnet, and outside its DHCP pool (or reserved), so it isn't handed to another device |
| Mask | The same mask the router gives its DHCP clients. Most home routers use `/24` (`255.255.255.0`); eero uses `/22` |
| Gateway and DNS | The router's own IP |

Getting any of these wrong gives either the "some devices can't reach it" problem above, or no connection at all.

**Static IP on a new router:**
1. Find the router's network. On a Windows PC connected to it, run `ipconfig` and note the IPv4 address, subnet mask and default gateway. For example, `192.168.1.23`, `255.255.255.0`, `192.168.1.1` means the network is `192.168.1.0/24`.
2. Pick a free address in that range, outside the router's DHCP pool, e.g. `192.168.1.45`.
3. Set it on the Pi:
   ```bash
   sudo nmcli con mod "<name>" ipv4.method manual ipv4.addresses 192.168.1.45/24 ipv4.gateway 192.168.1.1 ipv4.dns 192.168.1.1 && sudo nmcli con up "<name>"
   ```

**The more portable option is DHCP plus a router reservation.** Put the Pi on DHCP so the router always supplies the right mask, gateway and DNS:
```bash
sudo nmcli con mod "<name>" ipv4.method auto ipv4.addresses "" ipv4.gateway "" ipv4.dns "" && sudo nmcli con up "<name>"
```
Then reserve a fixed address for the Pi's MAC address in the router's settings. Nearly every router supports reservations, so this works anywhere without working out masks by hand.

**If the Pi's address changes,** also update the MQTT server address:
- on each controller's setup page, or in `secrets.h` before reflashing
- anywhere OpenHAB refers to the broker by IP

### Testing the broker from a PC
- **TCP reachability (PowerShell):** `Test-NetConnection 192.168.4.45 -Port 1883` should report `TcpTestSucceeded : True`.
- **Broker log on the Pi:** `sudo tail -f /var/log/mosquitto/mosquitto.log` shows connections and refusals. If a device's attempts never appear, it isn't reaching the Pi at all, a network problem rather than an MQTT one.

### Controller diagnostics
When an MQTT connection fails, the controllers print two lines on serial at 115200 baud, and show the reason on the OLED:
```
[MQTT] 192.168.4.45:1883 as MQTTGARAGEDOOR_ABCD, user '...' (password 10 chars): state -2 no TCP conn
[MQTT] WiFi '<ssid>' status 3 (3 = connected), IP 192.168.7.174, mask 255.255.252.0, gateway 192.168.4.1, RSSI -42, MAC ..., DHCP hostname 'GARAGE-DOOR' (setting 'GARAGE-DOOR')
```

| State | Meaning |
|---|---|
| `-2 no TCP conn` | Couldn't reach the broker: network, mask or firewall |
| `-4 timeout` | Reached something but got no MQTT reply (wrong port?) |
| `4 bad user/pass` | The broker rejected the login |
| `5 unauthorized` | Login OK but the broker's access rules refused this client |

Giving each controller an eero IP reservation keeps it at a known address. eero names a device after the first hostname it sees for that MAC and doesn't update it, so rename the device in the eero app if you reuse a controller for another project.
