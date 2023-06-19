#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char DEVICE_LOGIN_NAME[]  = "******-****-****-************";

const char SSID[]               = "*********";    // Network SSID (name)
const char PASS[]               = "*******";    // Network password (use for WPA, or use as key for WEP)
const char DEVICE_KEY[]  = "*******************";    // Secret device password

void onIftttCounterChange();
void onMaxspeedChange();
void onLockChange();

CloudCounter ifttt_counter;
float maxspeed;
bool lock;

void initProperties(){

  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(ifttt_counter, READWRITE, ON_CHANGE, onIftttCounterChange, 1);
  ArduinoCloud.addProperty(maxspeed, READWRITE, ON_CHANGE, onMaxspeedChange);
  ArduinoCloud.addProperty(lock, READWRITE, ON_CHANGE, onLockChange);

}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
