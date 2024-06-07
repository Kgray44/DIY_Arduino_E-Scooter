#define Flipsky_ESC //comment out if you are not using a Flipsky ESC

#include "DFRobot_UI.h"
#include "DFRobot_GDL.h"
#include <XPT2046_Touchscreen.h>
#include "DFRobot_Touch.h"

// PWM properties for motor driver
#ifdef Flipsky_ESC
const int freq = 45000;
#else
const int freq = 5000;
#endif
const int speedChannel = 0;
const int resolution = 8;

//Motor Driver
#define brakepin  16 //D11
#define cruisepin A4  // actually enable pin, so able to push scooter manually when on
#define speedpin 13 //D7 
//#define rpmpin A4
#define lowspeedpin //______ fill in pin number here!!!
#define highspeedpin //______ fill in pin number here!!!

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

//current sensor
#define currentpin A1

//brake switch
#define brakeswitchpin A3

//smoothing of throttle and GPS speed
const int numReadings = 10;
int readings[numReadings];  // the readings from the analog input
int readIndex = 0;          // the index of the current reading
int total = 0;              // the running total
const int numReadings2 = 2;
float readings2[numReadings2];  // the readings from the analog input
int readIndex2 = 0;          // the index of the current reading
float total2 = 0;              // the running total


//output voltage for motor driver flip.  Comment out to leave regular
//#define output_conversion

//brake switch
bool lastswitchvalue = false;

int throttleoffvalue = 0;
int throttleonvalue = 3300;
int throttleval = 0;
int lastthrottleval = 0;
int uppervalue = 100;

//Touch coordinates for calibration
#define touchxmin 290
#define touchxmax 3750
#define touchymin 300
#define touchymax 3700
#define backcolor COLOR_RGB565_WHITE
#define erasecolor backcolor//COLOR_RGB565_LGRAY
#define backgroundcolor COLOR_RGB565_CYAN

//gyro
float lastgyroreading = 0;
float readgyroint = 1;
float curgyroread = 0;
float gyroreadings = 5;
float gyrosum = 0;
float gyroy = 0;

//gnss / display
long readgnssinc = 1000;//time between GNSS readings
unsigned long lastreadgnss = 0;
long readdisplayinc = 1000;//time between display updates
unsigned long lastdisplay = 0;
sTim_t utc;
sTim_t date;
sLonLat_t latitude;
sLonLat_t longitude;
double altitude;
uint8_t satelites;
double knots;
double course;

sLonLat_t lastlongitude;
sLonLat_t lastlatitude;
double lastaltitude;
uint8_t lastsatelites;
double lastknots;
double lastcourse;
bool justsaved;



int motorvalue = 0;

//current sensor
int currentreadings = 10;//number of readings to average, more for a smoother more reliable response
long readcurrentint = 20;//time in milliseconds between battery readings
unsigned long lastcurreading = 0;
int curcurread = 0;
int cursum = 0;
float current = 0;                // the average
float watts = 0;

//AHT20
float temperature = 0;
float humidity = 0;
float HI = 0;

//Password
bool locked = false;//true
uint16_t gestures;
int passwordlength = 3;
#define EEPROM_SIZE passwordlength
int password[] = {GESTURE_UP,GESTURE_DOWN,GESTURE_LEFT};//Adjustable gesture password; can adjust password length (right now it is set to 3 in the "passwordlength" integer)
int passc = 0;//pass count

//battery
float batteryvolts = 0;
#define batterypin A2
int batteryreadings = 10;//number of readings to average, more for a smoother more reliable response
long readbatteryint = 20;//time in milliseconds between battery readings
unsigned long lastbatreading = 0;
long R1 = 15000;//15k ohms
long R2 = 1000;//1.1k ohms
int curbatread = 0;
int batsum = 0;
int adjbat = 0;
#define batteryamps 5
#define batteryvoltage 36 //voltage and amp hours of battery, or uncomment "batterywattage" below instead
//#define batterywattage 180
#define motorwattage 500 
#define maxcruisespeed 16 //mph

bool lastebrakeon = false;
unsigned long lastebrakechange = 0;
bool ebrakeon = false;
bool lockbtnon = false;
bool lastcruiseon = false;
unsigned long lastcruisechange = 0;
bool cruiseon = false;
float cruisesetspeed = 0;

int previousotherx1 = 0;
int previousothery1 = 0;
int previousotherx2 = 0;
int previousothery2 = 0;

bool more_visible = false;//turn the speed on the display black, for higher visibility

//BMI160
struct dat{
  float x = 0;
  float y = 0;
  float z = 0;
};
dat accel;
struct data{
  float x = 0;
  float y = 0;
  float z = 0;
};
data gyro;
float baseyangle = 0;

XPT2046_Touchscreen ts(TOUCH_CS);
DFRobot_Touch_XPT2046 touch (/* cs = */ TOUCH_CS);
DFRobot_ILI9341_240x320_HW_SPI screen(/*dc=*/TFT_DC,/*cs=*/TFT_CS,/*rst=*/TFT_RST);
