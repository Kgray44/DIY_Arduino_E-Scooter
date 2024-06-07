#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <ESPmDNS.h>
#include <WiFiAP.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>

#include "html.h"
//#include "settings.h"

const char* PARAM_INPUT_1 = "state";


//WiFi networks
const char SSID1[] = "NETWORK1";
const char PASS1[] = "PASSWORD1";
const char SSID2[] = "NETWORK2";
const char PASS2[] = "PASSWORD2";
const char SSID3[] = "";
const char PASS3[] = "";
const char *apssid = "escooter";
const char *appass = "AP_PASS_HERE";

int wifitimerlength = 7;//time in seconds before connection fails
int wifitimer = 0;
int wifismarttimer = 20;//time in seconds to wait for smartconfig
int wifistimer = 0;

unsigned long ota_progress_millis = 0;
unsigned long curmilll = 0;

int wakeup_status;

AsyncWebServer server(80);
WiFiMulti wifiMulti;

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "THROTTLEPLACEHOLDER"){
    String buttons ="";
    String throttlevaluetoprint = String(uppervalue);
    buttons+= "<h3>" + throttlevaluetoprint + "</h3>";
    return buttons;
  }
  return String();
}

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

float readThrottle(){
  throttleval = map(analogRead(throttlepin),throttleoffvalue,throttleonvalue,0,4095);
  return(throttleval);
  //Serial.println("Throttle: " + String(throttleval));
}

void WiFiStart(bool instaap = false){
  
  delay(2000);
  if (analogRead(throttlepin) > 100 || instaap){
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP(apssid,appass)){//,1,1)){//hidden
      Serial.println("Failed AP creation...");
    }
    else {
      Serial.println("AP created");
    }
    goto servers;
  }

  screen.setTextColor(COLOR_RGB565_BLACK);

  screen.fillScreen(COLOR_RGB565_YELLOW);
  screen.setCursor(50,125);
  screen.setTextSize(2);
  screen.print("Attempting connection");
  screen.setCursor(50,165);
  screen.println("of added networks...");
  //goto exi;
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(SSID1, PASS1);
  wifiMulti.addAP(SSID2, PASS2);
  wifiMulti.addAP(SSID3, PASS3);
  Serial.println("Connecting Wifi...");
  curmilll = millis();
  while (!(wifiMulti.run() == WL_CONNECTED)){
    if ((millis() - curmilll) > wifitimerlength*1000){
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
    screen.setCursor(50,50);
    screen.setTextSize(2);
    screen.print("WiFi successful!");
    screen.setCursor(50,90);
    screen.println("Connected to:");
    screen.setCursor(50,130);
    screen.println(WiFi.SSID());
    screen.setCursor(50,170);
    screen.print(WiFi.localIP());
    delay(5000);
  }
  else {
    screen.fillScreen(COLOR_RGB565_YELLOW);
    screen.setCursor(50,125);
    screen.setTextSize(2);
    screen.print("Waiting for ESPTouch...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();
    Serial.println("Waiting for SmartConfig.");
    curmilll = millis();
    while (!WiFi.smartConfigDone()) {
      if ((millis() - curmilll) > wifismarttimer*1000){
        goto exi;
      }
    }
    //Wait for WiFi to connect to AP
    screen.fillScreen(COLOR_RGB565_YELLOW);
    screen.setCursor(50,125);
    screen.setTextSize(2);
    screen.print("Connecting to ");
    screen.setCursor(50,165);
    screen.println("ESPTouch network...");
    Serial.println("Waiting for WiFi");
    curmilll = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if ((millis() - curmilll) > wifismarttimer*1000){
        goto exi;
      }
    }
    screen.fillScreen(COLOR_RGB565_GREEN);
    screen.setCursor(50,50);
    screen.setTextSize(2);
    screen.print("WiFi successful!");
    screen.setCursor(50,90);
    screen.println("Connected to:");
    screen.setCursor(50,130);
    screen.println(WiFi.SSID());
    screen.setCursor(50,170);
    screen.print(WiFi.localIP());
    delay(5000);
  }
  exi:
  if (wifiMulti.run() != WL_CONNECTED && WiFi.status() != WL_CONNECTED){
    screen.fillScreen(COLOR_RGB565_YELLOW);
    screen.setCursor(50,125);
    screen.setTextSize(2);
    screen.print("WiFi connection");
    screen.setCursor(50,165);
    screen.println("   failed");  
  }
  else {
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP(apssid,appass)){//,1,1)){//hidden
      Serial.println("Failed AP creation...");
    }
    else {
      Serial.println("AP created");
    }
  }
  
  Serial.println("");
  Serial.println("SmartConfig done.");

  //delay(1000);
  servers:
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/value?state=<inputMessage>
  server.on("/value", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/value?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      uppervalue = inputMessage.toInt();
      Serial.print("Received request:");
      Serial.println(uppervalue);
      EEPROM.write(3,uppervalue);
      EEPROM.write(4,wakeup_status);
      EEPROM.commit();
      ESP.restart();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(uppervalue).c_str());
  });

  //ElegantOTA.setAuth("admin", "maker");
  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  // Start server
  server.begin();
}

void WiFiRun(){
  /*if(wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
  }*/
  ElegantOTA.loop();
}
