/*
  MQTT Neopixel Stair lighting control
    By: Brian Smith
    Date: April 2025
    License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware License).
    
    Based on MQTT_Minimal framework also by Brian Smith

*/
#include <Arduino.h>
#include <WiFi.h>               //
#include <DNSServer.h>          // captive portal DNS for the setup AP
#include <ESPAsyncWebServer.h>  // https://github.com/me-no-dev/ESPAsyncWebServer
// for async use, edit this file, change #define ELEGANTOTA_USE_ASYNC_WEBSERVER 0 to 
// #define ELEGANTOTA_USE_ASYNC_WEBSERVER 1 (I put it outside the #ifndef typically)
// PrettyOTA looks nice, but it's not compatible with the Neopixel libraries
#include <ElegantOTA.h>         // https://github.com/ayushsharma82/ElegantOTA
#include <ESP32Time.h>          // https://github.com/fbiego/ESP32Time
#include <PubSubClient.h>       // https://github.com/knolleary/pubsubclient/releases/tag/v2.8
#include <Wire.h>               //
#include <Adafruit_GFX.h>       // https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h>   // https://github.com/adafruit/Adafruit_SSD1306
#include "FS.h"                 // 
#include <LittleFS.h>           // LittleFS_esp32 by loral from Arduino library manager
#include <ESP32Time.h>          // https://github.com/fbiego/ESP32Time
#include "MQTT_Minimal.h"       // common defines for the MQTT_Minial start point
#include "StairLights_V5.h"     // specific defines for this device
// forward declarations (the Arduino IDE generated these automatically for .ino files)
void WiFi_Setup();
void StartAP();
bool MQTT_Setup();
void MQTT_Subscribe();
void MQTT_Publish();
void MQTT_Publish(int topicID);
void LoadSetting(const char * path, String &value);
void LoadCredentials();
void WriteHTML();
void SaveCredentials();
void ApplyPendingSettings();
void UpdateLocalTime();
void LITTLEFS_Init();
String LITTLEFS_ReadFile(fs::FS &fs, const char * path);
void LITTLEFS_WriteFile(fs::FS &fs, const char * path, const char * message);
void LITTLEFS_AppendFile(fs::FS &fs, const char * path, const char * message);
void LITTLEFS_ListDir(fs::FS &fs, const char * dirname, uint8_t levels);
void LITTLEFS_DeleteFile(fs::FS &fs, const char * path);
void RefreshOLED(int fontSize);
void onOTAStart();
void onOTAProgress(size_t current, size_t final);
void onOTAEnd(bool success);
void ColorChangeTask(void * parameter);
void SetColors(uint8_t r, uint8_t g, uint8_t b, uint8_t bright);
// handle subscribed messages
void MQTT_Callback(char* topic, byte* message, unsigned int length) 
{ 
  #ifdef DEBUG_VERBOSE
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(" Message: ");
  #endif
  String messageTemp;
  for (int i = 0; i < length; i++) 
  {
    messageTemp += (char)message[i];
  }
  #ifdef DEBUG_VERBOSE
    Serial.print(messageTemp.c_str());
    Serial.println();
  #endif

  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.print("MQTT_Callback ");
    Serial.print(topic);
    Serial.print(" ");
    Serial.print(messageTemp.c_str());
    Serial.print(" (");
    Serial.print(length);
    Serial.println(")");
  #endif

  if(strcmp(topic, subscribed_topic[SUBSCRIBE_RED].c_str()) == 0)
  {
    redVal = atoi(messageTemp.c_str());
    redVal = map(redVal, 0, 100, 0, 255);
    lightingChange = true;
    #ifdef DEBUG_VERBOSE
      Serial.print("Red ");
      Serial.println(redVal);
    #endif  
  }
  else if(strcmp(topic, subscribed_topic[SUBSCRIBE_GREEN].c_str()) == 0)
  {
    greenVal = atoi(messageTemp.c_str());
    greenVal = map(greenVal, 0, 100, 0, 255);
    lightingChange = true;
    #ifdef DEBUG_VERBOSE
      Serial.print("Green ");
      Serial.println(greenVal);
    #endif
  }
  else if(strcmp(topic, subscribed_topic[SUBSCRIBE_BLUE].c_str()) == 0)
  {
    blueVal = atoi(messageTemp.c_str());
    blueVal = map(blueVal, 0, 100, 0, 255);
    lightingChange = true;
    #ifdef DEBUG_VERBOSE
      Serial.print("Blue ");
      Serial.println(blueVal);
    #endif
  }
  else if(strcmp(topic, subscribed_topic[SUBSCRIBE_BRIGHT].c_str()) == 0)
  {
    brightVal =  atoi(messageTemp.c_str());
    brightVal = map(brightVal, 0, 100, 0, MAX_BRIGHT);
    lightingChange = true;
    #ifdef DEBUG_VERBOSE
      Serial.print("Brightness ");
      Serial.println(brightVal);
    #endif
  }
  else if(strcmp(topic, subscribed_topic[SUBSCRIBE_PATTERN].c_str()) == 0)
  {
    patternVal =  atoi(messageTemp.c_str());
    lightingChange = true;
    #ifdef DEBUG_VERBOSE
      Serial.print("Pattern ");
      Serial.println(patternVal);
    #endif
  }
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.print("New pattern: ");
    Serial.println(patternVal);
    Serial.print("New color: 0x");
    Serial.print(redVal, HEX);
    Serial.print(" 0x");
    Serial.print(greenVal, HEX);
    Serial.print(" 0x");
    Serial.print(blueVal, HEX);
    Serial.print(" 0x");
    Serial.println(brightVal, HEX);
  #endif
  sprintf(tmpStr, "%d, %d, %d, %d, %d",redVal, greenVal, blueVal, brightVal, patternVal);
  oledString[2] = tmpStr;
  RefreshOLED(1);  
}
//
// main setup
//
void setup()
{
  Serial.begin(115200);
  delay(5000);
  Serial.println("MQTT minimal framework");
  Serial.println("======================");
  // OLED display setup
  if(!oledDisplay.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
  {
    #ifdef DEBUG_VERBOSE
      Serial.println(F("SSD1306 allocation failed"));
    #endif
    for(;;); // Don't proceed, loop forever
  }
  oledString[0] = "-----";
  oledString[1] = "-----";
  oledString[2] = "-----";
  oledString[3] = mqttDeviceName;
  RefreshOLED(1);

  // open LittleFS, format 
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.println("LITTLEFS Mount Failed");
    #endif
    oledString[0] = "LITTLEFS Mount Failed";
    RefreshOLED(1);
    return;
  }

  // load login credentials and other wifi/mqtt info
  // creates homepage with credential info
  LoadCredentials();

  if(strlen(mqtt_server.c_str()) == 0)
  {
    mqtt_server = "0.0.0.0";
  }
  #ifdef DEBUG_VERBOSE
    Serial.printf("MQTT Setup %s:%d\n", mqtt_server.c_str(), mqtt_port);
  #endif
  // MQTT client setup
  MQTT_client.setServer(mqtt_server.c_str(), mqtt_port);
  MQTT_client.setCallback(MQTT_Callback);

  if(strlen(WIFI_HOSTNAME.c_str()) == 0)
  {
    WIFI_HOSTNAME = "REPLACE_ME";
  }
  if(strlen(WIFI_SSID.c_str()) == 0)
  {
    WIFI_SSID = "REPLACE_ME";
  }
  if(strlen(WIFI_PASSWORD.c_str()) == 0)
  {
    WIFI_PASSWORD = "REPLACE_ME";
  }
  #ifdef DEBUG_VERBOSE
    Serial.printf("WiFi Setup user %s password %s host %s\n",WIFI_SSID.c_str(), WIFI_PASSWORD.c_str(), WIFI_HOSTNAME.c_str());
  #endif
  // Initialize WiFi
  connectMode = NOT_CONNECTED;
  WiFi.hostname(WIFI_HOSTNAME);
  #ifdef DEBUG_VERBOSE
    Serial.printf("start WiFi %s %s\n",WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
  #endif
  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
  // set up wifi in either STATION (if WiFi connects) or AP (no wifi connection)
  WiFi_Setup();
  // set up MQTT if in STATION mode
  randomSeed(analogRead(0));
  sprintf(tmpStr, "%s%c%c%c%c", mqttClientID.c_str(), (char)random(65, 90), (char)random(65, 90), (char)random(65, 90), (char)random(65, 90));
  mqttClientID = tmpStr;
  #ifdef DEBUG_VERBOSE
    Serial.printf("Random MQTT client name %s\n", tmpStr);
  #endif

  MQTT_Setup();

  UpdateLocalTime();
  published_payload[PUBLISH_TIME] = localTimeStr;
  published_payload[BOOT_TIME] = localTimeStr;
  MQTT_Publish(PUBLISH_TIME);
  MQTT_Publish(BOOT_TIME);

  // main page is credentials update
  // updates page shows Elegant OTA upload page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.printf("Request from webserver, send page %s\n", "/wifimanager.html");
    #endif
    request->send(LittleFS, "/wifimanager.html", "text/html");
  });
  // only the setup page and the OTA pages are served - none of the saved settings files
  // (/pass.txt, /mqttPas.txt, ...). Anything else: in AP mode, redirect to the setup page so
  // phones/laptops show it as a captive portal sign-in page; otherwise not found
  server.onNotFound([](AsyncWebServerRequest *request)
  {
    if(connectMode == AP)
    {
      request->redirect("http://" + apIP.toString() + "/");
    }
    else
    {
      request->send(404, "text/plain", "Not found");
    }
  });
  //
  // display web page and get credentials from user
  //  
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    // this runs on the web server's task - only copy the form values here; loop() applies
    // and saves them (flash writes + restart) once this response has reached the browser
    if(!settingsPending)
    {
      pendingParamCount = 0;
      for(size_t i = 0; (i < request->params()) && (pendingParamCount < MAX_PENDING_PARAMS); i++)
      {
        const AsyncWebParameter* p = request->getParam(i);
        if(p->isPost())
        {
          pendingParamName[pendingParamCount] = p->name();
          pendingParamValue[pendingParamCount] = p->value();
          pendingParamCount++;
        }
      }
      settingsPendingMillis = millis();
      settingsPending = true;
    }
    request->send(200, "text/plain", "Settings saved. ESP will restart, connect to your router and go to the device's IP address (shown on its display)");
  });

  // Initialize ElegantOTA
  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  // Start web server
  server.begin();
  redVal = 255;
  greenVal = 255;
  blueVal = 255;
  brightVal = 32;
  patternVal = 0;
  // create color change task on core 0
  // tried this to fix what ended up to be an incompatiblity between
  // PrettyOTA and the Neopixel libraries. Ultimatly changing back to 
  // ElegantOTA fixed it, so this may not be needed, but it can't hurt wither
  xTaskCreatePinnedToCore(
      ColorChangeTask, /* Function to implement the task */
      "ColorChangeTask", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &Task1,  /* Task handle. */
      0); /* Core where the task should run */
  lastMillis = millis();
}
//
// main loop
//
void loop()
{
  // settings submitted from the setup page - apply and save them (restarts) once the response has gone out
  if(settingsPending && (millis() - settingsPendingMillis > 1000))
  {
    ApplyPendingSettings();
  }
  // captive portal DNS (AP mode only)
  if(connectMode == AP)
  {
    dnsServer.processNextRequest();
  }
  // check reporting interval
  if(millis() - lastMillis > MQTT_REPORT_INTERVAL_MS)
  {
    lastMillis = millis();
    // check wifi
    WiFi_Setup();
    // nothing happens in AP mode, return
    if(connectMode == AP)
    {
      return;
    }
    // check MQTT
    if(MQTT_Setup())
    {
      // update local time
      UpdateLocalTime();
      // publish time only
      published_payload[PUBLISH_TIME] = localTimeStr;
      MQTT_Publish(PUBLISH_TIME);
      MQTT_Publish(BOOT_TIME);    
    }
  }
  if(lightingChange)
  {
    SetColors(redVal, greenVal, blueVal, brightVal);
  }
  ElegantOTA.loop();
  MQTT_client.loop();
}
// ================================ begin Wifi and config functions ================================
// set up wifi in STATION mode if wifi is available, or AP mode to provide setup access point 
void WiFi_Setup()
{
  // in AP mode (no credentials, or station connection failed) - leave the AP alone, since
  // loop() calls this every report interval and restarting the AP drops connected clients.
  // If there are saved credentials (e.g. the router was down when we tried), reboot every
  // AP_RETRY_MS to try WiFi again - but not while someone is connected to the setup page
  if (connectMode == AP)
  {
    if((WIFI_SSID != "REPLACE_ME") && (AP_RETRY_MS > 0) && (millis() - apStartMillis > AP_RETRY_MS))
    {
      if(WiFi.softAPgetStationNum() == 0)
      {
        Serial.println("[WiFi] AP mode timeout with no clients - restart to retry WiFi");
        oledString[0] = "Retry WiFi...";
        RefreshOLED(1);
        ESP.restart();
      }
      apStartMillis = millis();   // someone is on the setup page, check again later
    }
    return;
  }
  // no ssid name, go straight to AP mode
  if(WIFI_SSID == "REPLACE_ME")
  {
    #ifdef DEBUG_VERBOSE
      Serial.println("[WiFi] No SSID name from credentials");
    #endif
    StartAP();
    return;
  }
  // connected - first time through, switch to STATION mode
  if(WiFi.status() == WL_CONNECTED)
  {
    if(connectMode != STATION)
    {
      #ifdef DEBUG_VERBOSE
      Serial.println("in STATION mode");
      Serial.print("WiFi connected ");
      Serial.println(WiFi.localIP());
      #endif
      wifiIP = WiFi.localIP();
      sprintf(tmpStr, "STATION %s", WiFi.localIP().toString().c_str());
      oledString[0] = tmpStr;
      RefreshOLED(1);
      connectMode = STATION;
      #ifdef DEBUG_VERBOSE
      Serial.println("ElegantOTA can be accessed at: http://" + WiFi.localIP().toString() + "/update");
      #endif
    }
    return;
  }
  // not connected (never connected, or connection lost) - keep trying for up to
  // WIFI_CONNECT_TIMEOUT_MS, then fall back to AP mode so the setup page is reachable
  connectMode = NOT_CONNECTED;
  unsigned long startMillis = millis();
  int attemptCount = 0;
  while(WiFi.status() != WL_CONNECTED)
  {
    if(millis() - startMillis > WIFI_CONNECT_TIMEOUT_MS)
    {
      Serial.println("[WiFi] Station mode connection failed - fall back to AP mode...");
      StartAP();
      return;
    }
    Serial.printf("[WiFi] Connection attempt %d\n", attemptCount);
    sprintf(tmpStr, "WiFi attempt %d", attemptCount);
    oledString[0] = tmpStr;
    RefreshOLED(1);
    // the first attempt waits on the WiFi.begin() from setup() (or the automatic reconnect)
    if(attemptCount > 0)
    {
      WiFi.reconnect();
    }
    attemptCount++;
    WiFi.waitForConnectResult(WIFI_RETRY);
    // waitForConnectResult() returns early for some failures (e.g. SSID not found) - don't spin
    if(WiFi.status() != WL_CONNECTED)
    {
      delay(500);
    }
  }
  // connected - recursion sets STATION mode and updates the display
  WiFi_Setup();
}
// start the setup/OTA access point, named after the host
void StartAP()
{
  connectMode = AP;
  apStartMillis = millis();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_HOSTNAME.c_str());
  WiFi.softAPConfig(apIP, apGateway, apSubnet);
  // answer every DNS lookup with our address so the setup page opens automatically
  dnsServer.start(DNS_PORT, "*", apIP);
  wifiIP = WiFi.softAPIP();
  Serial.print("[WiFi] AP mode, connect to ");
  Serial.print(WIFI_HOSTNAME);
  Serial.print(" and go to http://");
  Serial.println(wifiIP);
  sprintf(tmpStr, "AP %s", wifiIP.toString().c_str());
  oledString[0] = tmpStr;
  RefreshOLED(1);
}
// ================================ end Wifi and config functions ================================
// ================================ begin MQTT functions ================================
// connect to MQTT server if not connected
bool MQTT_Setup()
{
  if(connectMode != STATION)
  {
    sprintf(tmpStr, "%s", "No MQTT in AP mode");
    oledString[1] = tmpStr;
    RefreshOLED(1);
    mqtt_connected = false;
    mqttConnectAttempts = 0;
    mqttFailStartMillis = 0;
    return false;
  }
  if(MQTT_client.connected())
  {
    mqtt_connected = true;
    mqttConnectAttempts = 0;
    mqttFailStartMillis = 0;
    return true;
  }
  // try to connect to MQTT
  MQTT_client.connect(mqttClientID.c_str(), mqtt_user.c_str(), mqtt_password.c_str());
  if(MQTT_client.connected())
  {
    oledString[1] = mqttClientID;
    RefreshOLED(1);

    #ifdef DEBUG_EXTRA_VERBOSE
    Serial.println();
    Serial.println("MQTT connected");
    Serial.println("==============");
    #endif
    // set up MQTT subscriptions
    // also lists published topics
    MQTT_Subscribe();
    #ifdef DEBUG_EXTRA_VERBOSE
    Serial.println("MQTT ready");
    #endif
    mqtt_connected = true;
    mqttConnectAttempts = 0;
    mqttFailStartMillis = 0;
  }
  else
  {
    // time failures from the first one since the last good connection
    if(mqttConnectAttempts == 0)
    {
      mqttFailStartMillis = millis();
    }
    mqttConnectAttempts++;
    sprintf(tmpStr, "No MQTT (%d)", mqttConnectAttempts);
    oledString[1] = tmpStr;
    RefreshOLED(1);
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.println();
      Serial.printf("%s state %d\n", tmpStr, MQTT_client.state());
    #endif
    mqtt_connected = false;
    // can't reach the broker (wrong server/user/password, or broker down) - fall back to
    // AP mode so the settings can be checked on the setup page. With saved WiFi credentials
    // the AP mode retry (AP_RETRY_MS) reboots later to try again
    if(millis() - mqttFailStartMillis > MQTT_FAIL_TIMEOUT_MS)
    {
      Serial.printf("[MQTT] no connection after %d attempts - fall back to AP mode...\n", mqttConnectAttempts);
      MQTT_client.disconnect();
      mqttConnectAttempts = 0;
      mqttFailStartMillis = 0;
      oledString[1] = "MQTT failed";
      StartAP();
    }
  }
  return mqtt_connected;
}
// set up subscriptions
void MQTT_Subscribe()
{
  #ifdef DEBUG_EXTRA_VERBOSE
  Serial.printf("Subscribing to %d topics\n", MAX_SUBSCRIBE);
  #endif
  for(int idx = 0; idx < MAX_SUBSCRIBE; idx++)
  {
    #ifdef DEBUG_EXTRA_VERBOSE
    Serial.printf("\tSubscribing to %s\n", subscribed_topic[idx]);
    #endif
    MQTT_client.subscribe(subscribed_topic[idx].c_str());
  }
  #ifdef DEBUG_EXTRA_VERBOSE
  Serial.printf("Publishing %d topics\n", MAX_PUBLISH);
  for(int idx = 0; idx < MAX_PUBLISH; idx++)
  {
    Serial.printf("\tPublishing to %s\n", published_topic[idx].c_str());
  }
  #endif
}
// publish all topics
void MQTT_Publish()
{
  int pubSubResult = 0;
  for(int idx = 0; idx < MAX_PUBLISH; idx++)
  {
    MQTT_Publish(idx);
  }
  MQTT_client.loop();
}
// publish specific topic
void MQTT_Publish(int topicID)
{
  int pubSubResult = 0;
  #ifdef DEBUG_VERBOSE
    Serial.print("publishing ");
    Serial.println(published_topic[topicID]);
  #endif
  pubSubResult = MQTT_client.publish(published_topic[topicID].c_str(), 
                                     published_payload[topicID].c_str(), 
                                     published_payload[topicID].length());
  #ifdef DEBUG_VERBOSE
    Serial.print((pubSubResult == 0 ? "FAIL TO PUBLISH Topic: " : "PUBLISHED Topic: "));
    Serial.print(published_topic[topicID]);
    Serial.print(" Payload: ");
    Serial.println(published_payload[topicID]);
  #endif
  MQTT_client.loop();
}
// ================================ end MQTT functions ================================
// ================================ begin credentials and LITTLEFS functions ================================
// load one setting from LITTLEFS - leaves the default in place if the file was never written
void LoadSetting(const char * path, String &value)
{
  if(!LittleFS.exists(path))
  {
    #ifdef DEBUG_VERBOSE
      Serial.printf("[Settings] %s not saved, using default\n", path);
    #endif
    return;
  }
  value = LITTLEFS_ReadFile(LittleFS, path);
  value.trim();
  #ifdef DEBUG_VERBOSE
    Serial.printf("[Settings] %s loaded (%d chars)\n", path, value.length());
  #endif
}
// load wifi credentials from LITTLEFS
void LoadCredentials()
{
  // settings never saved from the setup page keep their StairLights_V5.h defaults
  LoadSetting(ssidPath, WIFI_SSID);
  LoadSetting(passPath, WIFI_PASSWORD);
  LoadSetting(clientPath, WIFI_HOSTNAME);
  LoadSetting(tzPath, tz);
  LoadSetting(dstPath, dst);

  gmtOffset_sec = atoi(tz.c_str()) * 3600; // convert hours to seconds
  daylightOffset_sec = atoi(dst.c_str()) * 3600; // convert hours to seconds

  LoadSetting(mqtt_serverPath, mqtt_server);
  mqttserverIP.fromString(mqtt_server.c_str());
  if(LittleFS.exists(mqtt_portPath))
  {
    String mqtt_portStr = LITTLEFS_ReadFile(LittleFS, mqtt_portPath);
    mqtt_portStr.trim();
    mqtt_port = atoi(mqtt_portStr.c_str());
  }
  if(mqtt_port == 0)
  {
    mqtt_port = 1883;
  }
  LoadSetting(mqtt_userPath, mqtt_user);
  LoadSetting(mqtt_passwordPath, mqtt_password);
  LITTLEFS_ReadFile(LittleFS, "/wifimanager.html");
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.print("SSID: ");
    Serial.println(WIFI_SSID);
    Serial.print("PASSWORD: ");
    Serial.println(WIFI_PASSWORD);
    Serial.print("Timezone offset: ");
    Serial.print(tz);
    Serial.print(" ");
    Serial.println(gmtOffset_sec);
    Serial.print("DST offset: ");
    Serial.print(dst);
    Serial.print(" ");
    Serial.println(daylightOffset_sec);

    Serial.println("MQTT credentials");
    Serial.print("Server: ");
    Serial.println(mqtt_server);
    Serial.print("Port: ");
    Serial.println(mqtt_port);
    Serial.print("User: ");
    Serial.println(mqtt_user);
    Serial.print("Password: ");
    Serial.println(mqtt_password);
  #endif
  WriteHTML();
}
void WriteHTML()
{
  // write main html page
  char tempStr[32];
  LITTLEFS_WriteFile(LittleFS, pagePath, "<!DOCTYPE html>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "<html>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "<head>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "  <title>");
  LITTLEFS_AppendFile(LittleFS, pagePath, WIFI_HOSTNAME.c_str());
  LITTLEFS_AppendFile(LittleFS, pagePath, " Credentials</title>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  LITTLEFS_AppendFile(LittleFS, pagePath, "  <link rel=\"icon\" href=\"data:,\">");
  LITTLEFS_AppendFile(LittleFS, pagePath, "  <link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">");
  LITTLEFS_AppendFile(LittleFS, pagePath, "</head>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "<body>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "  <div class=\"topnav\">");
  //LITTLEFS_AppendFile(LittleFS, pagePath, "    <h1>Sump Monitor Credentials</h1>");
  sprintf(tmpStr, "    <h1>%s Credentials</h1>", WIFI_HOSTNAME.c_str());
  LITTLEFS_AppendFile(LittleFS, pagePath, tmpStr);
  LITTLEFS_AppendFile(LittleFS, pagePath, "  </div>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "  <div class=\"content\">");
  LITTLEFS_AppendFile(LittleFS, pagePath, "    <div class=\"card-grid\">");
  LITTLEFS_AppendFile(LittleFS, pagePath, "      <div class=\"card\">");
  LITTLEFS_AppendFile(LittleFS, pagePath, "        <form action=\"/\" method=\"POST\">");
  LITTLEFS_AppendFile(LittleFS, pagePath, "          <p>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "			<div>WiFi Credentials</div>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "            <label for=\"ssid\">SSID</label>");
  sprintf(tmpStr, "            <input type=\"text\" id =\"ssid\" name=\"ssid\" value=\"%s\"><br>",  WIFI_SSID.c_str());
  LITTLEFS_AppendFile(LittleFS, pagePath, tmpStr);

  LITTLEFS_AppendFile(LittleFS, pagePath, "            <label for=\"pass\">Password</label>");
  // passwords are never written into the page - blank keeps the current one
  strcpy(tmpStr, "            <input type=\"password\" id =\"pass\" name=\"pass\" placeholder=\"leave blank to keep current\"><br>");
  LITTLEFS_AppendFile(LittleFS, pagePath, tmpStr);

  LITTLEFS_AppendFile(LittleFS, pagePath, "            <label for=\"clientname\">Host Name</label>");
  sprintf(tmpStr, "            <input type=\"text\" id =\"clientname\" name=\"clientname\" value=\"%s\"><br>", WIFI_HOSTNAME.c_str());
  LITTLEFS_AppendFile(LittleFS, pagePath, tmpStr);
  
  LITTLEFS_AppendFile(LittleFS, pagePath, "			<hr/>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "			<div>MQTT setup</div>");

  LITTLEFS_AppendFile(LittleFS, pagePath, "			<label for=\"mqtt_serverIP\">MQTT server IP</label>");
  sprintf(tmpStr, "            <input type=\"text\" id =\"mqtt_serverIP\" name=\"mqtt_serverIP\" value=\"%s\"><br>", mqtt_server.c_str());
  LITTLEFS_AppendFile(LittleFS, pagePath, tmpStr);

  LITTLEFS_AppendFile(LittleFS, pagePath, "            <label for=\"mqtt_port\">MQTT port</label>");
  sprintf(tmpStr, "            <input type=\"text\" id =\"mqtt_port\" name=\"mqtt_port\" value=\"%d\"><br>", (int)mqtt_port);
  LITTLEFS_AppendFile(LittleFS, pagePath, tmpStr);

  LITTLEFS_AppendFile(LittleFS, pagePath, "            <label for=\"mqtt_user\">MQTT user name</label>");
  sprintf(tmpStr, "            <input type=\"text\" id =\"mqtt_user\" name=\"mqtt_user\" value=\"%s\"><br>", mqtt_user.c_str());
  LITTLEFS_AppendFile(LittleFS, pagePath, tmpStr);

  LITTLEFS_AppendFile(LittleFS, pagePath, "            <label for=\"mqtt_password\">MQTT password</label>");
  strcpy(tmpStr, "            <input type=\"password\" id =\"mqtt_password\" name=\"mqtt_password\" placeholder=\"leave blank to keep current\"><br>");
  LITTLEFS_AppendFile(LittleFS, pagePath, tmpStr);

  LITTLEFS_AppendFile(LittleFS, pagePath, "			<hr/>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "			<div>Time zone and DST offsets (hours)</div>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "			<label for=\"tz\">Time Zone offset from GST</label>");
  sprintf(tmpStr, "            <input type=\"text\" id =\"timezone\" name=\"timezone\" value=\"%s\"><br>", tz.c_str());
  LITTLEFS_AppendFile(LittleFS, pagePath, tmpStr);
  
  LITTLEFS_AppendFile(LittleFS, pagePath, "            <label for=\"dst\">DST offset</label>");
  sprintf(tmpStr, "            <input type=\"text\" id =\"dst\" name=\"dst\" value=\"%s\"><br>", dst.c_str());
  LITTLEFS_AppendFile(LittleFS, pagePath, tmpStr);
  LITTLEFS_AppendFile(LittleFS, pagePath, "			<hr/>");

  LITTLEFS_AppendFile(LittleFS, pagePath, "            <input type =\"submit\" value =\"Submit\">");
  // type="button" so it doesn't submit (save + restart) the settings form
  strcpy(tmpStr, "          <button type=\"button\" onclick=\"location.href='/update'\">OTA</button>");
  LITTLEFS_AppendFile(LittleFS, pagePath, tmpStr);

  LITTLEFS_AppendFile(LittleFS, pagePath, "          </p>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "        </form>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "      </div>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "    </div>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "  </div>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "</body>");
  LITTLEFS_AppendFile(LittleFS, pagePath, "</html>");
}
// apply the values submitted from the setup page (collected by the HTTP_POST handler), save and restart
void ApplyPendingSettings()
{
  #ifdef DEBUG_VERBOSE
    Serial.printf("[Settings] applying %d values from the setup page\n", pendingParamCount);
  #endif
  for(int idx = 0; idx < pendingParamCount; idx++)
  {
    const String &name = pendingParamName[idx];
    const String &value = pendingParamValue[idx];
    if (name == PARAM_INPUT_1)
    {
      WIFI_SSID = value;
    }
    else if (name == PARAM_INPUT_2)
    {
      // blank = keep the current password (the setup page never shows it)
      if(value.length())
      {
        WIFI_PASSWORD = value;
      }
    }
    else if (name == PARAM_INPUT_3)
    {
      WIFI_HOSTNAME = value;
    }
    else if (name == PARAM_INPUT_4)
    {
      tz = value;
    }
    else if (name == PARAM_INPUT_5)
    {
      dst = value;
    }
    else if (name == PARAM_INPUT_6)
    {
      mqtt_server = value;
    }
    else if (name == PARAM_INPUT_7)
    {
      mqtt_port = atoi(value.c_str());
      if(mqtt_port == 0)
      {
        mqtt_port = 1883;
      }
    }
    else if (name == PARAM_INPUT_8)
    {
      mqtt_user = value;
    }
    else if (name == PARAM_INPUT_9)
    {
      // blank = keep the current password (the setup page never shows it)
      if(value.length())
      {
        mqtt_password = value;
      }
    }
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.printf("%s %s\n", name.c_str(), value.c_str());
    #endif
  }
  SaveCredentials();
}
// save credentials to files
void SaveCredentials()
{
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.println("SaveCredentials()");
  #endif
  LITTLEFS_WriteFile(LittleFS, ssidPath, WIFI_SSID.c_str());
  LITTLEFS_WriteFile(LittleFS, passPath, WIFI_PASSWORD.c_str());
  LITTLEFS_WriteFile(LittleFS, clientPath, WIFI_HOSTNAME.c_str());
  LITTLEFS_WriteFile(LittleFS, tzPath, tz.c_str());
  LITTLEFS_WriteFile(LittleFS, dstPath, dst.c_str());
  LITTLEFS_WriteFile(LittleFS, mqtt_serverPath, mqtt_server.c_str());
  sprintf(mqtt_portStr, "%d", mqtt_port);
  LITTLEFS_WriteFile(LittleFS, mqtt_portPath, mqtt_portStr);
  LITTLEFS_WriteFile(LittleFS, mqtt_userPath, mqtt_user.c_str());
  LITTLEFS_WriteFile(LittleFS, mqtt_passwordPath, mqtt_password.c_str());
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.print("SSID set to: ");
    Serial.println(WIFI_SSID);
    Serial.print("Password set to: ");
    Serial.println(WIFI_PASSWORD);
    Serial.print("IP Address: ");
    Serial.println(wifiIP.toString());
    Serial.print("Time zone offset set to: ");
    Serial.println(tz);
    Serial.print("DST offset set to: ");
    Serial.println(dst);
    Serial.print("MQTT server IP: ");
    Serial.println(mqtt_server);
    Serial.print("MQTT port: ");
    Serial.println(mqtt_port);
    Serial.print("MQTT username: ");
    Serial.println(mqtt_user);
    Serial.print("MQTT password: ");
    Serial.println(mqtt_password);
  #endif
  #ifdef DEBUG_VERBOSE
    // read back what was written so the serial log shows whether the save worked
    Serial.printf("[Settings] saved - read back SSID '%s' host '%s' MQTT %s:%s user '%s'\n",
                  LITTLEFS_ReadFile(LittleFS, ssidPath).c_str(),
                  LITTLEFS_ReadFile(LittleFS, clientPath).c_str(),
                  LITTLEFS_ReadFile(LittleFS, mqtt_serverPath).c_str(),
                  LITTLEFS_ReadFile(LittleFS, mqtt_portPath).c_str(),
                  LITTLEFS_ReadFile(LittleFS, mqtt_userPath).c_str());
  #endif
  LittleFS.end();
  delay(500);
  ESP.restart();
}
// ================================ end WiFi initialize/credentials functions ================================
// ================================ begin NTP/RTC time functions ================================
// get local time (initially set via NTP server, after set get from ESP32 RTC)
void UpdateLocalTime()
{
  if((connectMode != STATION) && (!rtcTimeSet))
  {
    sprintf(localTimeStr, "Time not set, wifi not connected");
    return;
  }
  // if not set from NTP, get time and set RTC
  if(!rtcTimeSet)
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.print("Time from NTP server ");
    #endif
    struct tm timeinfo;
    //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    configTime(gmtOffset_sec, daylightOffset_sec, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
    // getLocalTime() waits up to WIFI_RETRY ms per attempt; give up after a few so an
    // unreachable NTP server can't hang the device - the next call will retry
    int ntpAttempts = 0;
    while(!getLocalTime(&timeinfo, WIFI_RETRY))
    {
      #ifdef DEBUG_EXTRA_VERBOSE
        Serial.println("failed to get time from NTP");
      #endif
      if(++ntpAttempts >= NTP_RETRY_COUNT)
      {
        sprintf(localTimeStr, "Time not set, no NTP response");
        return;
      }
    }
    //GET DATE
    strftime(dayMonth, sizeof(dayMonth), "%d", &timeinfo);
    strftime(monthName, sizeof(monthName), "%b", &timeinfo);
    strftime(year, sizeof(year), "%Y", &timeinfo);
    dayNum = atoi(dayMonth);
    monthNum = timeinfo.tm_mon+1;
    yearNum = atoi(year);

    //GET TIME
    strftime(hour, sizeof(hour), "%H", &timeinfo);
    strftime(minute, sizeof(minute), "%M", &timeinfo);
    strftime(second, sizeof(second), "%S", &timeinfo);
    hourNum = atoi(hour);
    minNum = atoi(minute);
    secondNum = atoi(second);

    //rtc.setTime(secondNum, minNum, hourNum, dayNum, monthNum, yearNum);
    rtc.setTimeStruct(timeinfo);
    rtcTimeSet = true;

    lastMinNum = minNum;
    lastHourNum = hourNum;
    lastDayNum = dayNum;
  }
  // use RTC for time
  else
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.print("get time from local RTC...");
    #endif
    // read all fields from one snapshot so they can't straddle a second/minute rollover
    struct tm timeinfo = rtc.getTimeStruct();
    dayNum = timeinfo.tm_mday;
    monthNum = timeinfo.tm_mon + 1;
    yearNum = timeinfo.tm_year + 1900;
    hourNum = timeinfo.tm_hour;   // 24-hour (rtc.getHour() defaults to 12-hour)
    minNum = timeinfo.tm_min;
    secondNum = timeinfo.tm_sec;
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.print("...time retrieved from local RTC ");
    #endif
  }
  // set last time values to current
  if(lastMinNum == -1)
  {
    lastMinNum = minNum;
    lastHourNum = hourNum;
    lastDayNum = dayNum;
  }
  sprintf(localTimeStr, "%02d/%02d/%04d %02d:%02d:%02d", monthNum, dayNum, yearNum, hourNum, minNum, secondNum);
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.print("localTimeStr = ");
    Serial.println(localTimeStr);
  #endif
  if(!connectDateTimeSet)
  {
    strcpy(connectTimeStr, localTimeStr);
    connectDateTimeSet = true;
  }
}
// ================================ end NTP/RTC time functions ================================
// ================================ begin LITTLEFS functions ================================
//
// Initialize LITTLEFS
void LITTLEFS_Init() 
{
  if (!LittleFS.begin(true)) 
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.println("An error has occurred while mounting LITTLEFS");
    #endif
    return;
  }
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.println("LITTLEFS mounted successfully");
  #endif
}
// Read File from LITTLEFS
String LITTLEFS_ReadFile(fs::FS &fs, const char * path)
{
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.printf("Reading file: %s\n", path);
  #endif
  File file = fs.open(path);
  if(!file || file.isDirectory())
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.println("failed to open file for reading");
    #endif
    return String();
  }
  
  String fileContent;
  while(file.available())
  {
    fileContent = file.readStringUntil('\n');
  }
  file.close();
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.println("file read");
    Serial.println("returning file content");
  #endif
  return fileContent;
}
// Write to LITTLEFS file
void LITTLEFS_WriteFile(fs::FS &fs, const char * path, const char * message)
{
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.printf("Writing >>%s<< to file: %s\n", message, path);
  #endif
  File file = fs.open(path, FILE_WRITE);
  if(!file)
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.println("failed to open file for writing");
    #endif
    return;
  }
  if(!file.print(message))
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.println("- file write failed");
    #endif
  }
  file.close();
}
// Append to LITTLEFS file
void LITTLEFS_AppendFile(fs::FS &fs, const char * path, const char * message)
{
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.println(message);
  #endif
  File file = fs.open(path, FILE_APPEND);
  if(!file)
  {
    #ifdef DEBUG_DEBUG_EXTRA_VERBOSE
      Serial.println("failed to open file for writing");
    #endif
    return;
  }
  if(!file.print(message))
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.println("- file write failed");
    #endif
  }
  file.close();
}
// list LITTLEFS files
void LITTLEFS_ListDir(fs::FS &fs, const char * dirname, uint8_t levels)
{
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.printf("Listing directory: %s\r\n", dirname);
  #endif
  File root = fs.open(dirname);
  if(!root)
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.println("failed to open directory");
    #endif
    return;
  }
  if(!root.isDirectory())
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.println(" - not a directory");
    #endif
    return;
  }

  File file = root.openNextFile();
  while(file)
  {
    if(file.isDirectory())
    {
      #ifdef DEBUG_EXTRA_VERBOSE
        Serial.print("  DIR : ");
        Serial.println(file.name());
      #endif
      if(levels)
      {
        LITTLEFS_ListDir(fs, file.name(), levels -1);
      }
    }
    else 
    {
      #ifdef DEBUG_EXTRA_VERBOSE
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("\tSIZE: ");
        Serial.println(file.size());
      #endif
    }
    file = root.openNextFile();
  }
}
// delete named file from LITTLEFS
void LITTLEFS_DeleteFile(fs::FS &fs, const char * path)
{
  #ifdef DEBUG_EXTRA_VERBOSE
    Serial.printf("Deleting file: %s\n", path);
  #endif
  if(fs.remove(path))
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.println("file deleted");
    #endif
  }
  else 
  {
    #ifdef DEBUG_EXTRA_VERBOSE
      Serial.println("delete failed");
    #endif
  }
}
// ================================ end credentials and LITTLEFS functions ================================
// ================================ begin OLED display functions ================================
// update OLED display
void RefreshOLED(int fontSize)
{
  oledDisplay.clearDisplay();
  oledDisplay.setTextSize(fontSize);             // Draw 2X-scale text
  oledDisplay.setTextColor(SSD1306_WHITE);
  int y = 0;
  for(int idx = 0; idx < 4; idx++)
  {
    oledDisplay.setCursor(0,y);             // Start at top-left corner
    oledDisplay.print(oledString[idx].c_str());
    y+= 16;
  }
  oledDisplay.display();
}
// ================================ end OLED display functions ================================
// ================================ begin ElegantOTA support functions ========================================
void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}
// ================================ end ElegantOTA support functions ================================
// ================================ begin device specific functions ========================================
//
void ColorChangeTask( void * parameter)
{
  Serial.printf("ColorChangeTask on core %d\n", xPortGetCoreID());
  //neoPixelStrip = Adafruit_NeoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ400);
  //neoPixelStrip = Freenove_ESP32_WS2812(NUMPIXELS, NEO_PIN, NEO_CHANNEL, TYPE_GRB);
  neoPixelStrip.begin();
  Serial.printf("neoPixelStrip started on core %d\n", xPortGetCoreID());
  while(true)
  {
    if(lightingChange)
    {
      SetColors(redVal, greenVal, blueVal, brightVal);
    }
  }
}
//
void SetColors(uint8_t r, uint8_t g, uint8_t b, uint8_t bright)
{
  #ifdef DEBUG_VERBOSE
    Serial.printf("SetColor(%d, %d, %d, %d) on core %d\n", r, g, b, bright, xPortGetCoreID());
  #endif
  redVal = r;
  greenVal = g;
  blueVal = b;
  brightVal = bright;
  neoPixelStrip.setBrightness(bright);
  for(int pixelIdx = 0; pixelIdx < NUMPIXELS; pixelIdx++)
  {
    //neoPixelStrip.setPixelColor(pixelIdx, r, g, b);  // adafruit library
    neoPixelStrip.setLedColorData(pixelIdx, r, g, b);  // Freenove library
  }
  #ifdef DEBUG_VERBOSE
    Serial.println("neoPixelStrip.show();");
  #endif
  neoPixelStrip.show();
  sprintf(tmpStr, "R%d G%d B%d Br%d",r, g, b, bright);
  oledString[2] = tmpStr;
  //RefreshOLED(1);  
  #ifdef DEBUG_VERBOSE
    Serial.println("set lightingChange = false;");
  #endif
  lightingChange = false;
}
