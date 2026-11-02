/*
  MQTT device starter framework
  By: Brian Smith
  Date: April 2025
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware License).

  DeviceSpecific.h - add device specific defines, members, library includes, etc here
*/
// WiFi/MQTT credentials live in secrets.h, which is not committed to git
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #error "include/secrets.h is missing - copy include/secrets.example.h to include/secrets.h and fill in your WiFi/MQTT settings"
#endif
// battery powered devices: uncomment for light sleep when idle and WiFi modem sleep. Needs the
// Arduino-as-ESP-IDF-component build - copy platformio.ini, sdkconfig.defaults, partitions.csv
// and components/ from SlidingDoorLock
//#define LOW_POWER
//#define LOW_POWER_MAX_MHZ 80               // CPU speed while awake
//#define LOW_POWER_MIN_MHZ 40               // lowest CPU speed before light sleep (ESP32: 40 = crystal)
//#define LOW_POWER_LOOP_DELAY_MS 50         // loop() idles this long each pass so the CPU can sleep
//#define LOW_POWER_WIFI_PS WIFI_PS_MIN_MODEM // WIFI_PS_MAX_MODEM saves more but can miss broadcasts (ARP)
//#define MQTT_KEEPALIVE_S 120               // fewer keepalive pings than PubSubClient's 15 s default

// WiFi
// firmware version shown on the setup page - bump when releasing
#define FW_VERSION "1.0.0"
String WIFI_HOSTNAME = "MQTT-MINIMAL";
String WIFI_SSID     = SECRET_WIFI_SSID;
String WIFI_PASSWORD = SECRET_WIFI_PASSWORD;
IPAddress wifiIP(0, 0, 0, 0);
// soft AP (setup/OTA access point) address - any private subnet that differs from the home LAN
IPAddress apIP(192, 168, 42, 99);
IPAddress apGateway(192, 168, 42, 99);
IPAddress apSubnet(255, 255, 255, 0);

// MQTT
String mqttClientID = "MQTTMINIMAL_";   // local name, must be unique on MQTT server so append a random string at the end
String mqttDeviceName = "MQTTMINIMAL";
#define MQTT_CONNECT_DELAY 30000
#define MQTT_REPORT_INTERVAL_MS 2000       // minimum time between checking sensors and publishing changes to MQTT server
#define MQTT_TIMEREPORT_INTERVAL_MS 60000  // minimum time between publishing time-only updates to MQTT server
String mqtt_server = SECRET_MQTT_SERVER;
String mqtt_user = SECRET_MQTT_USER;
String mqtt_password = SECRET_MQTT_PASSWORD;

// ----- device specific pubsub topics
// subscribed (listening) topic(s)
// list of text strings for subscribed topics from server, if any
#define MAX_SUBSCRIBE 1
String subscribed_topic[MAX_SUBSCRIBE] =  { "MQTT_Minimal/Command" };
// defines for subscribed topics - more descriptive than array indices
#define SUBSCRIBE_COMMAND 0
// published topics to send to server
#define MAX_PUBLISH 2
String published_topic[MAX_PUBLISH] = { "MQTT_Minimal/LastUpdate",
                                        "MQTT_Minimal/BootTime"
 };
// add defines here to avoid remembering what the index means - optional
#define PUBLISH_TIME 0
#define BOOT_TIME 1
String published_payload[MAX_PUBLISH];

// sensor/control specific
// pin definitions
