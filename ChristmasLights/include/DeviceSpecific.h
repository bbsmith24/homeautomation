/*
  MQTT device starter framework
  By: Brian Smith
  Date: April 2025
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware License).

  DeviceSpecific.h - add device specific defines, members, library includes, etc here
*/
#include <SparkFunSX1509.h>

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
String mqttClientID = "MQTTXMAS_";   // local name, must be unique on MQTT server so append a random string at the end
String mqttDeviceName = "MQTTXMAS_"; // device name shown on the setup page
#define MQTT_CONNECT_DELAY 30000
#define MQTT_REPORT_INTERVAL_MS 2000       // minimum time between checking sensors and publishing changes to MQTT server
#define MQTT_TIMEREPORT_INTERVAL_MS 60000  // minimum time between publishing time-only updates to MQTT server
String mqtt_server = SECRET_MQTT_SERVER;
String mqtt_user = SECRET_MQTT_USER;
String mqtt_password = SECRET_MQTT_PASSWORD;
// command is for all devices, this specifies which part of the message this device uses - the rest is ignored
int messageSegment = 0;  // 0 = first 16 bits, 1 = second 16 bits, 2 = third 16 bits

// ----- device specific pubsub topics
// subscribed (listening) topic(s)
// list of text strings for subscribed topics from server, if any
#define MAX_SUBSCRIBE 1
String subscribed_topic[MAX_SUBSCRIBE] =  { "MQTTXMAS/Command" };
// defines for subscribed topics - more descriptive than array indices
#define SUBSCRIBE_COMMAND 1
// published topics to send to server
#define MAX_PUBLISH 2
String published_topic[MAX_PUBLISH] = { "MQTTXMAS/LastUpdate",
                                        "MQTTXMAS/BootTime"
 };
// SX1509 I/O expander
#define ACTIVE_BOARDS 1
#define DEFINED_BOARDS 2
#define DEFINED_OUTPUTS 16
 SX1509 io[DEFINED_BOARDS];                                  // Create an SX1509 objects
const byte SX1509_ADDRESS[DEFINED_BOARDS] = {0x3E, 0x3F};   // SX1509 I2C addresses
const byte SX1509_LED_PIN[DEFINED_BOARDS][DEFINED_OUTPUTS] = {           // pin indexes
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, // SX1509 #1 pins
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15} // SX1509 #2 pins
};
// add defines here to avoid remembering what the index means - optional
#define PUBLISH_TIME 0
#define BOOT_TIME 1
String published_payload[MAX_PUBLISH];
void UpdateLights(String command);
// sensor/control specific
// pin definitions
