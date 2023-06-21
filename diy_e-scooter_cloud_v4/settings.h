//Arduino Cloud
const char DEVICE_LOGIN_NAME[]  = "44d31cdc-7f98-4fd6-ada9-a1592b4eb71a";
const char DEVICE_KEY[]  = "XFZYO2ZSQKE4JQQQSZWB";    // Secret device password

//WiFi networks
const char SSID1[] = "Apple Network 85064d";
const char PASS1[] = "12344321";
const char SSID2[] = "";
const char PASS2[] = "";
const char SSID3[] = "";
const char PASS3[] = "";

//Motor Driver
#define brakepin 2 //D9
#define cruisepin 16 //D11 // actually enable pin, so able to push scooter manually when on
#define speedpin 13 //D7
#define rpmpin A4

//Display
#define TFT_DC  25 //D2
#define TFT_RST 26 //D3
#define TFT_CS  14 //D6
#define TOUCH_CS 4 //D12
#define TFT_BL  12 //D13

//Throttle
#define throttlepin A0 //throttle pin

//LEDs
#define LEDPIN 17//D10

//battery
#define batterypin A2

//Password
int password[] = {GESTURE_UP,GESTURE_DOWN,GESTURE_LEFT};//Adjustable gesture password; can adjust password length (right now it is set to 3 in the "passwordlength" integer)

