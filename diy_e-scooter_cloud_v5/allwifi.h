#include "WiFi.h"
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

void onIftttCounterChange();
void onMaxspeedChange();
void onLockChange();

//WiFi
int wifitimerlength = 15000;//time in milliseconds before connection fails
int wifitimer = 0;
int wifismarttimer = 60;//time in seconds to wait for smartconfig
int wifistimer = 0;
WiFiConnectionHandler* conn = nullptr;

//Arduino cloud
CloudCounter ifttt_counter;
float maxspeed;
bool lock;

char NSSID[32+1];
char NPASS[63+1];

void WiFiStart(){
  screen.setTextColor(COLOR_RGB565_BLACK);

  screen.fillScreen(COLOR_RGB565_YELLOW);
  screen.setCursor(50,125);
  screen.setTextSize(2);
  screen.print("Attempting connection");
  screen.println(" of added networks...");
  //goto exi;
  wifiMulti.addAP(SSID1, PASS1);
  wifiMulti.addAP(SSID2, PASS2);
  wifiMulti.addAP(SSID3, PASS3);
  if (analogRead(throttlepin) > 100){goto exi;}
  Serial.println("Connecting Wifi...");
  while (!(wifiMulti.run() == WL_CONNECTED)){
    wifitimer++;
    delay(1);
    if (analogRead(throttlepin) > 100){goto exi;}
    if (wifitimer > wifitimerlength){
      goto ex;
    }
  }
  ex:
  if(wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    screen.fillScreen(COLOR_RGB565_GREEN);
    screen.setCursor(50,125);
    screen.setTextSize(2);
    screen.print("WiFi successful!");
    screen.println("Connected to:");
    screen.println(WiFi.SSID());
    delay(1000);
  }
  else {
    if (analogRead(throttlepin) > 100){goto exi;}
    screen.fillScreen(COLOR_RGB565_YELLOW);
    screen.setCursor(50,125);
    screen.setTextSize(2);
    screen.print("Waiting for ESPTouch...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();
    Serial.println("Waiting for SmartConfig.");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      if (analogRead(throttlepin) > 100){goto exi;}
      Serial.print(".");
      wifistimer++;
      if (wifistimer > wifismarttimer){
        goto exi;
      }
    }
    //Wait for WiFi to connect to AP
    screen.fillScreen(COLOR_RGB565_YELLOW);
    screen.setCursor(50,125);
    screen.setTextSize(2);
    screen.print("Connecting to ");
    screen.println("ESPTouch network...");
    Serial.println("Waiting for WiFi");
    if (analogRead(throttlepin) > 100){goto exi;}
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (analogRead(throttlepin) > 100){goto exi;}
      wifitimer++;
      if (wifitimer > wifismarttimer*2){
        goto exi;
      }
    }
    screen.fillScreen(COLOR_RGB565_GREEN);
    screen.setCursor(50,125);
    screen.setTextSize(2);
    screen.print("WiFi successful!");
    screen.println("Connected to:");
    screen.println(WiFi.SSID());
    Serial.println("WiFi Connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
  exi:
  if (wifiMulti.run() != WL_CONNECTED && WiFi.status() != WL_CONNECTED){
    screen.fillScreen(COLOR_RGB565_YELLOW);
    screen.setCursor(50,125);
    screen.setTextSize(2);
    screen.print("WiFi connection");
    screen.println("   failed");
  }
 
  Serial.println("");
  Serial.println("SmartConfig done.");

  delay(1000);

  initProperties();

  // Connect to Arduino IoT Cloud
  strcpy(NSSID, WiFi.SSID().c_str());
  strcpy(NPASS, WiFi.psk().c_str());
  conn = new WiFiConnectionHandler(NSSID, NPASS);
  ArduinoCloud.begin(*conn);//ArduinoIoTPreferredConnection);

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  
  OTAStart();
}

void OTAStart(){
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
   ArduinoOTA.setHostname("EscooterESP");
  // No authentication by default
   ArduinoOTA.setPassword("maker");//(const char 

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiRun(){
  if(wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
  }
  ArduinoOTA.handle();
  ArduinoCloud.update();
}

void onIftttCounterChange()  {
  // Add your code here to act upon IftttCounter change
}

void initProperties(){
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(ifttt_counter, READWRITE, ON_CHANGE, onIftttCounterChange, 1);
  ArduinoCloud.addProperty(maxspeed, READWRITE, ON_CHANGE, onMaxspeedChange);
  ArduinoCloud.addProperty(lock, READWRITE, ON_CHANGE, onLockChange);
}
