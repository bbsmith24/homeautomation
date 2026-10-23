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
#define FW_VERSION "5.1.0"
String WIFI_HOSTNAME = "SUMP-MONITOR";
String WIFI_SSID     = SECRET_WIFI_SSID;
String WIFI_PASSWORD = SECRET_WIFI_PASSWORD;
IPAddress wifiIP(0, 0, 0, 0);
// soft AP (setup/OTA access point) address - any private subnet that differs from the home LAN
IPAddress apIP(192, 168, 42, 1);
IPAddress apGateway(192, 168, 42, 1);
IPAddress apSubnet(255, 255, 255, 0);

// MQTT
String mqttClientID = "MQTTSumpMonitor_";   // local name, must be unique on MQTT server so append a random string at the end
String mqttDeviceName = "SUMP-MONITOR";
#define MQTT_CONNECT_DELAY 30000
#define MQTT_REPORT_INTERVAL_MS 2000
String mqtt_server = SECRET_MQTT_SERVER;
String mqtt_user = SECRET_MQTT_USER;
String mqtt_password = SECRET_MQTT_PASSWORD;

// ----- device specific pubsub topics
// subscribed (listening) topic(s)
// list of text strings for subscribed topics from server, if any
#define MAX_SUBSCRIBE 0
// defines for subscribed topics - more descriptive than array indices
#define SUBSCRIBE_TOPIC 0
String subscribed_topic[1] =  { "" }; // use MAX_SUBSCRIBE if > 0
// published topics to send to server
#define MAX_PUBLISH 12
// add defines here to avoid remembering what the index means - optional
#define PUBLISH_LOWSWITCH 0
#define PUBLISH_MIDSWITCH 1
#define PUBLISH_HIGHSWITCH 2
#define PUBLISH_LOWCONTACT 3
#define PUBLISH_MIDCONTACT 4
#define PUBLISH_HIGHCONTACT 5
#define PUBLISH_MAINCYCLE 6
#define PUBLISH_BACKUPCYCLE 7
#define PUBLISH_FLOODCYCLE 8
#define PUBLISH_STATE 9
#define BOOT_TIME 10
#define PUBLISH_TIME 11
String published_topic[MAX_PUBLISH] = { "Sump_Pump/LowSwitch",
                                        "Sump_Pump/MidSwitch",
                                        "Sump_Pump/HighSwitch",
                                        "Sump_Pump/LowContact",
                                        "Sump_Pump/MidContact",
                                        "Sump_Pump/HighContact",
                                        "Sump_Pump/MainCycleTime",
                                        "Sump_Pump/BackupCycleTime",
                                        "Sump_Pump/FloodCycleTime",
                                        "Sump_Pump/State",
                                        "Sump_Pump/BootTime",
                                        "Sump_Pump/LastUpdate"};
String published_payload[MAX_PUBLISH];

// sump monitor specific
// epochTime for start, last cycle elapsed time at end of cycle
unsigned long cycleStartTimes[3] = {0 ,0, 0}; // start time for each cycle
float cycleTimes[3] = {0.0, 0.0, 0.0};        // elapsed time for each cycle

bool neverUpdated = true;
unsigned long lastUpdate = 0;

#define LOW_DETECT_PIN 12
#define MID_DETECT_PIN 14
#define HIGH_DETECT_PIN 27

// state machine
// from initial to whatever the switches represent
// state changes
// BELOW_SUMP -> ABOVE_SUMP
// ABOVE_SUMP -> BELOW_SUMP (normal, main pump cycle)
// ABOVE_SUMP -> ABOVE_BACKUP (main pump failure)
// ABOVE_BACKUP -> ABOVE_SUMP (backup pump cycle)
// ABOVE_BACKUP -> ABOVE_FLOOD (backup pump failure)
// ABOVE_FLOOD -> ABOVE_BACKUP (water receding, backup pump caught up, something)
//
#define INITIAL_STATE 0  // unknown at startup
#define BELOW_SUMP    1  // below sump float (all open)
#define ABOVE_SUMP    2  // above sump float (low closed, mid and high open)
#define ABOVE_BACKUP  3  // above backup float (low and mid closed, high open)
#define ABOVE_FLOOD   4  // above flood float (low mid and high)
#define ERROR_STATE   5  // illegal transition - stuck switch?

String states[6] = {"INITIAL_STATE", "BELOW_SUMP", "ABOVE_SUMP", "ABOVE_BACKUP", "ABOVE_FLOOD", "ERROR_STATE" };
int current_state = INITIAL_STATE;
int next_state = INITIAL_STATE;