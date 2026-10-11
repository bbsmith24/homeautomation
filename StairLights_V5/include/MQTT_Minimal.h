/*
  MQTT device starter framework
  By: Brian Smith
  Date: April 2025
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware License).

  DMQTT_Minimal.h - core functionality
*/
// debugging
//#define DEBUG_VERBOSE
//#define DEBUG_EXTRA_VERBOSE
// wifi
#define WIFI_RETRY 5000L
#define WIFI_CONNECT_TIMEOUT_MS 60000L  // give up on WiFi and start the setup AP after this long
#define AP_RETRY_MS 300000L             // in AP mode with saved credentials, reboot to retry WiFi after this long (0 = never)
unsigned long apStartMillis = 0;
#define MQTT_FAIL_TIMEOUT_MS 120000L    // no MQTT connection for this long -> fall back to the setup AP
enum WiFiConnectMode
{
  STATION, 
  AP,
  NOT_CONNECTED
};
WiFiConnectMode connectMode = NOT_CONNECTED;
WiFiClient espClient;
PubSubClient MQTT_client(espClient);
AsyncWebServer server(80); // Server on port 80 (HTTP)

// MQTT
uint16_t mqtt_port = 0;
char mqtt_portStr[32];
IPAddress mqttserverIP(0, 0, 0, 0);
bool mqtt_connected = false;
int mqttConnectAttempts = 0;
unsigned long mqttFailStartMillis = 0;

// LITTLEFS and setup files
#define FORMAT_LITTLEFS_IF_FAILED true
// setup page path
#define pagePath     "/wifimanager.html"
// File paths to save input values permanently
#define ssidPath     "/ssid.txt"
#define passPath     "/pass.txt"
#define clientPath   "/client.txt"       // wifi client node name
#define mqtt_serverPath    "/mqttIP.txt"
#define mqtt_portPath      "/mqttPrt.txt"
#define mqtt_userPath      "/mqttUse.txt"
#define mqtt_passwordPath  "/mqttPas.txt"
#define tzPath       "/tz.txt"
#define dstPath      "/dst.txt"
// get info from setup page
#define PARAM_INPUT_1  "ssid"
#define PARAM_INPUT_2  "pass"
#define PARAM_INPUT_3  "clientname"
#define PARAM_INPUT_4  "timezone"
#define PARAM_INPUT_5  "dst"
#define PARAM_INPUT_6   "mqtt_serverIP"
#define PARAM_INPUT_7   "mqtt_port"
#define PARAM_INPUT_8   "mqtt_user"
#define PARAM_INPUT_9  "mqtt_password"
// setup page values received by the web server task, applied and saved by loop()
#define MAX_PENDING_PARAMS 16
String pendingParamName[MAX_PENDING_PARAMS];
String pendingParamValue[MAX_PENDING_PARAMS];
int pendingParamCount = 0;
volatile bool settingsPending = false;
unsigned long settingsPendingMillis = 0;
// captive portal - in AP mode every DNS name resolves to the device so the setup page opens automatically
#define DNS_PORT 53
DNSServer dnsServer;

// RTC and NTP time
#define ntpServer "pool.ntp.org"
#define NTP_RETRY_COUNT 3   // NTP attempts (WIFI_RETRY ms each) before giving up until the next call
ESP32Time rtc(0);
String tz;
String dst;
int dayNum;
int monthNum;
int yearNum;
int hourNum;
int minNum;
int secondNum;
int lastDayNum = -1;
int lastHourNum = -1;
int lastMinNum = -1;
char weekDay[10];
char dayMonth[4];
char monthName[5];
char year[6];
char hour[4];
char minute[4];
char second[4];
char localTimeStr[256];
char connectTimeStr[256];
bool connectDateTimeSet = false;
bool rtcTimeSet = false;
long  gmtOffset_sec = 0;
int   daylightOffset_sec = 0;

// OLED display
String oledString[4] = {"Line 1", "Line 2", "Line 3", "Line 4"};
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 oledDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// temporary string, last millis check
unsigned long lastMillis = 0;
char tmpStr[256];

// Elegant OTA
unsigned long ota_progress_millis = 0;

// tasks
TaskHandle_t Task1;