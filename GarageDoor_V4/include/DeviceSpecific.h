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
// WiFi
// firmware version shown on the setup page - bump when releasing
#define FW_VERSION "4.1.0"
String WIFI_HOSTNAME = "GARAGE-DOOR";
String WIFI_SSID     = SECRET_WIFI_SSID;
String WIFI_PASSWORD = SECRET_WIFI_PASSWORD;
IPAddress wifiIP(0, 0, 0, 0);
// soft AP (setup/OTA access point) address - any private subnet that differs from the home LAN
IPAddress apIP(192, 168, 42, 99);
IPAddress apGateway(192, 168, 42, 99);
IPAddress apSubnet(255, 255, 255, 0);

// MQTT
String mqttClientID = "MQTTGARAGEDOOR_";   // local name, must be unique on MQTT server so append a random string at the end
String mqttDeviceName = "GARAGEDOOR";
#define MQTT_CONNECT_DELAY 30000
#define MQTT_REPORT_INTERVAL_MS 2000
String mqtt_server = SECRET_MQTT_SERVER;
String mqtt_user = SECRET_MQTT_USER;
String mqtt_password = SECRET_MQTT_PASSWORD;

// ----- device specific pubsub topics
// subscribed (listening) topic(s)
// list of text strings for subscribed topics from server, if any
#define MAX_SUBSCRIBE 2
String subscribed_topic[MAX_SUBSCRIBE] =  { "Garage_Door/GarageDoorButton",
                                            "MQTT_Sensors/TestMQTTRequest"};
// defines for subscribed topics - more descriptive than array indices
#define SUBSCRIBE_DOOR_BUTTON 0
#define SUBSCRIBE_TEST_REQUEST 1
// published topics to send to server
#define MAX_PUBLISH 5
String published_topic[MAX_PUBLISH] = { "Garage_Door/ClosedContact",
                                        "Garage_Door/OpenContact",
                                        "Garage_Door/Status",
                                        "Garage_Door/LastUpdate",
                                        "Garage_Door/BootTime"
 };
// add defines here to avoid remembering what the index means - optional
#define PUBLISH_CLOSED 0
#define PUBLISH_OPEN 1
#define PUBLISH_STATUS 2
#define PUBLISH_TIME 3
#define BOOT_TIME 4
String published_payload[MAX_PUBLISH];

// sensor/control specific
// pin definitions
#define MQTT_STATUS_LED    0   // on when MQTT is connected
#define EXT_RESET          2   // low to reset (labeled SDA on Thing)
#define WIFI_STATUS_LED    5   // on when WIFI is connected (also built in LED)
#define OPENER_BUTTON_PIN 12   // trigger the transistor to close the door switch connection
#define CLOSE_DETECT_PIN  13   // state of the door closed switch
#define OPEN_DETECT_PIN   14   // state of the door open switch
#define VBATT_PIN  A0
String strDoorValue;
bool neverUpdated = true;

