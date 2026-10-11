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
String WIFI_HOSTNAME = "STAIR-LIGHTS";
String WIFI_SSID     = SECRET_WIFI_SSID;
String WIFI_PASSWORD = SECRET_WIFI_PASSWORD;
IPAddress wifiIP(0, 0, 0, 0);
// soft AP (setup/OTA access point) address - any private subnet that differs from the home LAN
IPAddress apIP(192, 168, 42, 1);
IPAddress apGateway(192, 168, 42, 1);
IPAddress apSubnet(255, 255, 255, 0);

// MQTT
String mqttClientID = "MQTTSTAIRLIGHTS_";   // local name, must be unique on MQTT server so append a random string at the end
String mqttDeviceName = "STAIRLIGHTS";
#define MQTT_CONNECT_DELAY 30000
#define MQTT_REPORT_INTERVAL_MS 2000
String mqtt_server = SECRET_MQTT_SERVER;
String mqtt_user = SECRET_MQTT_USER;
String mqtt_password = SECRET_MQTT_PASSWORD;

// ----- device specific pubsub topics
// subscribed (listening) topic(s)
#define MAX_SUBSCRIBE 5
// defines for subscribed topics - more descriptive than array indices
#define SUBSCRIBE_RED 0
#define SUBSCRIBE_GREEN 1
#define SUBSCRIBE_BLUE 2
#define SUBSCRIBE_BRIGHT 3
#define SUBSCRIBE_PATTERN 4
String subscribed_topic[MAX_SUBSCRIBE] =  { "Stair_Lights/RedVal",
                                            "Stair_Lights/GreenVal",
                                            "Stair_Lights/BlueVal",
                                            "Stair_Lights/Brightness",
                                            "Stair_Lights/Pattern" };
// published topics to send to server
#define MAX_PUBLISH 4
// add defines here to avoid remembering what the index means - optional
#define PUBLISH_TIME 0
#define PUBLISH_ADD_PATTERN_NAME 1
#define PUBLISH_REQUEST_SETTINGS 2
#define BOOT_TIME 3

String published_topic[MAX_PUBLISH] = { "Stair_Lights/LastUpdate",
                                        "Stair_Lights/AddPatternName",
                                        "Stair_Lights/RequestSettings",
                                        "Stair_Lights/BootTime" };
String published_payload[MAX_PUBLISH];

// device specific
// works with the Freenove library, may work with adafruit too
//#include <Adafruit_NeoPixel.h>  // https://github.com/adafruit/Adafruit_NeoPixel
#include "Freenove_WS2812_Lib_for_ESP32.h"  // https://github.com/Freenove/Freenove_WS2812_Lib_for_ESP32
// neopixel settings

#define NUMPIXELS    14  // NeoPixels in strip
#define NEO_PIN      12  // NeoPixel control pin
#define MAX_BRIGHT  255  // max brightness of 0-255 range
#define NEO_CHANNEL		1

//Adafruit_NeoPixel neoPixelStrip;// = Adafruit_NeoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ400);
Freenove_ESP32_WS2812 neoPixelStrip = Freenove_ESP32_WS2812(NUMPIXELS, NEO_PIN, NEO_CHANNEL, TYPE_GRB);

uint8_t redVal;
uint8_t greenVal;
uint8_t blueVal;
uint8_t brightVal;
uint8_t patternVal;
bool lightingChange = false;



