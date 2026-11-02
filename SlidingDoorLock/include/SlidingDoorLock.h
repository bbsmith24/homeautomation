/*
  MQTT Sliding Door Lock
  By: Brian Smith
  Date: November 2026
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware License).

  SlidingDoorLock.h - device specific defines, members, library includes, etc
*/
// WiFi/MQTT credentials live in secrets.h, which is not committed to git
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #error "include/secrets.h is missing - copy include/secrets.example.h to include/secrets.h and fill in your WiFi/MQTT settings"
#endif
// battery powered - light sleep when idle, WiFi modem sleep (see the low power functions in main.cpp)
#define LOW_POWER
#define LOW_POWER_MAX_MHZ 80               // CPU speed while awake
#define LOW_POWER_MIN_MHZ 40               // lowest CPU speed before light sleep (ESP32: 40 = crystal)
#define LOW_POWER_LOOP_DELAY_MS 50         // loop() idles this long each pass so the CPU can sleep
// WIFI_PS_MIN_MODEM wakes for every DTIM beacon, so broadcasts (ARP) are never missed.
// WIFI_PS_MAX_MODEM sleeps longer (listen interval) and saves more, but can miss broadcasts
#define LOW_POWER_WIFI_PS WIFI_PS_MIN_MODEM
#define MQTT_KEEPALIVE_S 120               // fewer keepalive pings than PubSubClient's 15 s default

// WiFi
// firmware version shown on the setup page - bump when releasing
#define FW_VERSION "1.0.0"
String WIFI_HOSTNAME = "SLIDING-DOOR-LOCK";
String WIFI_SSID     = SECRET_WIFI_SSID;
String WIFI_PASSWORD = SECRET_WIFI_PASSWORD;
IPAddress wifiIP(0, 0, 0, 0);
// soft AP (setup/OTA access point) address - any private subnet that differs from the home LAN
IPAddress apIP(192, 168, 42, 99);
IPAddress apGateway(192, 168, 42, 99);
IPAddress apSubnet(255, 255, 255, 0);

// MQTT
String mqttClientID = "MQTTSLIDINGDOORLOCK_";   // local name, must be unique on MQTT server so append a random string at the end
String mqttDeviceName = "SLIDINGDOORLOCK";
#define MQTT_CONNECT_DELAY 30000
#define MQTT_REPORT_INTERVAL_MS 10000      // minimum time between checking battery/lock state and publishing changes to MQTT server
#define MQTT_TIMEREPORT_INTERVAL_MS 900000 // minimum time between publishing time-only updates to MQTT server (15 min, saves battery)
String mqtt_server = SECRET_MQTT_SERVER;
String mqtt_user = SECRET_MQTT_USER;
String mqtt_password = SECRET_MQTT_PASSWORD;

// ----- device specific pubsub topics
// subscribed (listening) topic(s)
// list of text strings for subscribed topics from server, if any
#define MAX_SUBSCRIBE 1
String subscribed_topic[MAX_SUBSCRIBE] =  { "Sliding_Door_Lock/Command" };   // LOCK/UNLOCK (or ON/OFF)
// defines for subscribed topics - more descriptive than array indices
#define SUBSCRIBE_LOCK_COMMAND 0
// published topics to send to server
#define MAX_PUBLISH 4
String published_topic[MAX_PUBLISH] = { "Sliding_Door_Lock/Status",
                                        "Sliding_Door_Lock/Battery",
                                        "Sliding_Door_Lock/LastUpdate",
                                        "Sliding_Door_Lock/BootTime"
 };
// add defines here to avoid remembering what the index means - optional
#define PUBLISH_STATUS 0
#define PUBLISH_BATTERY 1
#define PUBLISH_TIME 2
#define BOOT_TIME 3
String published_payload[MAX_PUBLISH];

// sensor/control specific
// pin definitions
#define SERVO_PWM_PIN     25   // servo signal
#define SERVO_POWER_PIN   26   // servo power switch (high side P-MOSFET driver) - servo is unpowered between moves
#define SERVO_POWER_ON  HIGH   // SERVO_POWER_PIN level that powers the servo
#define BATTERY_PIN       36   // A0 (ADC1) - battery voltage through BATTERY_DIVIDER
// servo
#define SERVO_LEDC_CHANNEL 0
#define SERVO_PWM_HZ      50
#define SERVO_PWM_BITS    16
#define SERVO_LOCKED_US   2000 // pulse width for the locked position - adjust to the door
#define SERVO_UNLOCKED_US 1000 // pulse width for the unlocked position - adjust to the door
#define SERVO_MOVE_MS     1500 // time the servo is powered for each move
#define AUTO_LOCK_MS         0 // relock this long after unlocking (0 = never)
// battery: 1M from battery to BATTERY_PIN, 470k from BATTERY_PIN to ground (4.4 uA at 6.4 V)
#define BATTERY_DIVIDER   ((1000.0 + 470.0) / 470.0)
#define BATTERY_SAMPLES   8
#define BATTERY_REPORT_DELTA_V 0.05  // publish the battery voltage when it moves this much
// OLED - optional on a battery device; turned off this long after boot to save power (0 = always on)
#define OLED_ON_MS        120000
// lock state - saved so it survives restarts
#define lockStatePath     "/lock.txt"
#define LOCK_LOCKED       "Locked"
#define LOCK_UNLOCKED     "Unlocked"
#define LOCK_UNKNOWN      "Unknown"
String lockState = LOCK_UNKNOWN;
String lockRequest = "";       // set by MQTT_Callback, carried out by loop()
bool lockChanged = false;
unsigned long unlockMillis = 0;
float lastReportedBattery = -1.0;
bool oledOff = false;
