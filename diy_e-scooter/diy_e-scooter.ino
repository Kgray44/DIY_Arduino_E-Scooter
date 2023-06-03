#include "DFRobot_GDL.h"
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include "DFRobot_GNSS.h"
#include "DFRobot_BMI160.h"
#include "DFRobot_AHT20.h"
#include "DFRobot_GR10_30.h"
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

//Testing
bool testing = false;

//Motor Driver
#define brakepin 16 //D11
#define cruisepin 4 //D12 // actually enable pin, so able to push scooter manually when on
#define speedpin 13 //D7
#define rpmpin A4
// PWM properties for motor driver
const int freq = 5000;
const int speedChannel = 0;
const int resolution = 8;

//Display
#define TFT_DC  25 //D2
#define TFT_CS  14 //D6
#define TFT_RST 26 //D3
#define TFT_BL  12 //D13
#define TOUCH_CS 4 //D12

//Throttle
#define throttlepin A0 //throttle pin
int throttleoffvalue = 0;
int throttleonvalue = 4095;
int throttleval = 0;

//Touch coordinates for calibration
#define touchxmin 290
#define touchxmax 3750
#define touchymin 300
#define touchymax 3700
#define backcolor COLOR_RGB565_WHITE
#define erasecolor backcolor//COLOR_RGB565_LGRAY
#define backgroundcolor COLOR_RGB565_CYAN

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

//AHT20
float temperature = 0;
float humidity = 0;

//WS2812
#define LEDPIN 5
#define NUMPIXELS 19
float lastcoursereading = 0;
float coursechangetime = 1000;
float lastcoursereadtime = 0;
float courseturnsignalinc = 50; //degrees of turning for turning signal to turn on
float lastspeedreading = 0;
float speedchangetime = 1000;
float lastspeedreadtime = 0;
float speedbrakeinc = -0.5;//decrease in speed before brake light turns on
int leftturn1 = 0;
int leftturn2 = 1;
int leftturn3 = 2;
int rightturn3 = 2;
int rightturn2 = 3;
int rightturn1 = 4;
int brake1 = 16;
int brake2 = 17;
int brake3 = 18;
int brake4 = 19;
unsigned long startmillis = 0;
int animationcount = 1;

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
#define batteryamps 5
#define batteryvoltage 36 //voltage and amp hours of battery, or uncomment "batterywattage" below instead
//#define batterywattage 180
#define motorwattage 350 
#define maxcruisespeed 15 //mph //20

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

XPT2046_Touchscreen ts(TOUCH_CS);
DFRobot_ILI9341_240x320_HW_SPI screen(/*dc=*/TFT_DC,/*cs=*/TFT_CS,/*rst=*/TFT_RST);
DFRobot_GNSS_I2C gnss(&Wire,GNSS_DEVICE_ADDR);
DFRobot_BMI160 bmi160;
const int8_t i2c_addr = 0x69;
DFRobot_AHT20 aht20;
DFRobot_GR10_30 gr10_30(/*addr = */GR10_30_DEVICE_ADDR, /*pWire = */&Wire);
Adafruit_NeoPixel pixels(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

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

void setup() {
  digitalWrite(speedpin, LOW);
  pinMode(rpmpin, INPUT);

  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);
  for (int i=0;i<passwordlength;i++){//read password from EEPROM
    password[i] = EEPROM.read(i);    
  }
  
  pinMode(batterypin, INPUT);
  pinMode(throttlepin, INPUT);
  pinMode(brakepin, OUTPUT);
  pinMode(cruisepin, OUTPUT);
  pinMode(speedpin, OUTPUT);
  digitalWrite(speedpin, LOW);
  pinMode(rpmpin, INPUT);
  attachInterrupt(rpmpin, interruptRoutine, FALLING);
  ledcSetup(speedChannel, freq, resolution);
  ledcAttachPin(speedpin, speedChannel);

  if(!gnss.begin()){
    Serial.println("NO GPS device !");
    delay(1000);
  }

  gnss.enablePower();
  gnss.setGnss(eGPS_BeiDou_GLONASS);
  gnss.setRgbOn();

  if (bmi160.softReset() != BMI160_OK){
    Serial.println("bmi160 reset false");
    //while(1);
  }
  if (bmi160.I2cInit(i2c_addr) != BMI160_OK){
    Serial.println("bmi160 init false");
    //while(1);
  }
  readAccelGyro();
  baseyangle = gyro.y;
  //baseyangle = 20;

  uint8_t status;
  if((status = aht20.begin()) != 0){
    Serial.print("AHT20 sensor initialization failed. error status : ");
    Serial.println(status);
    delay(1000);
  }
  
  pixels.begin();
  pixels.clear();
  startGestureSensor();
  ts.begin();
  ts.setRotation(3);
  
  screen.begin();
  screen.setRotation(3);
  startDisplay();
}

void loop() {
  if (locked){
    lockedloop();
    return;
  }
  if (ts.touched()){
    TS_Point p = ts.getPoint();
    handleTouch(abs(map(p.x,touchxmax,touchxmin,0,320)),abs(map(p.y,touchymax,touchymin,0,240)));
  }
  if ((millis() - lastreadgnss) > readgnssinc){
    readGNSS();
    lastreadgnss = millis();
  }
  readAccelGyro();
  readTempHum();
  readBattery();
  readThrottle();
  if (((millis() - lastdisplay) > readdisplayinc) && (satelites != 0)){
    handleSlowDisplay();
    lastdisplay = millis();
  }
  handleFastDisplay();
  handleDisplayButtons();
  float handleAngleLineValue = (baseyangle*100) - (gyro.y*100);
  handleAngleLine(handleAngleLineValue);
  handleGestures();
  handleMotordriver();
  handleLeds();
}

void handleTouch(int x,int y){
  Serial.print("Touch X = " + String(x));
  Serial.println(" | Touch Y = " + String(y));

  if (x >= 215 && x <= 320){
    if (y < 45){
      changepassword();
    }
    else if (y >= 45 && y <= 104){
      if ((millis() - lastebrakechange) > 1000){//wait 1 second between reading button as pressed
        if (knots < 5){
          ebrakeon = !ebrakeon;
          lastebrakechange = millis();
        }
      }
    }  
    else if (y >= 105 && y <= 160){
      if ((millis() - lastcruisechange) > 1000){//wait 1 second between reading button as pressed
        cruiseon = !cruiseon;
        cruisesetspeed = knots;
        lastcruisechange = millis();
      }
    }  
  }
  else if (x >= 5 && x <= 105){
    if (y < 45){
      changepassword();
    }
    else if (y >= 45 && y <= 105){
      if (!lockbtnon){
        lockbtnon = true;
      }
      else {
        screen.setTextSize(2);
        screen.setCursor(15,115);
        screen.setTextColor(COLOR_RGB565_RED);
        screen.print("Unlock with gestures!");//display unlock warning
        delay(3000);
        screen.fillRect(14, 114, 290, 20, erasecolor);//erase display unlock warning
      }
    }
  }
}

void handleSlowDisplay(){
  //satelites
  screen.setTextSize(2);
  screen.setCursor(287,15);
  screen.fillRect(286, 13, 26, 18, erasecolor);//backcolor
  if (satelites <= 5){
    screen.setTextColor(COLOR_RGB565_RED);
  }
  else if (satelites > 5 && satelites <= 8){
    screen.setTextColor(COLOR_RGB565_ORANGE);
  }
  else if (satelites > 8){
    screen.setTextColor(COLOR_RGB565_GREEN);
  }
  screen.print(String(satelites));
  //Serial.println(satelites);

  //speed (knots)
  screen.setTextSize(4);
  if (knots >= 10){
    screen.setCursor(100,62);//118,62
    screen.fillRect(92, 58, 132, 37, erasecolor);//117, 57, 90, 30,
  }
  else {
    screen.setCursor(110,62);//118,62
    screen.fillRect(100, 58, 105, 37, erasecolor);//117, 57, 90, 30,
  }
  if (knots <= 1.00){
    screen.setTextColor(COLOR_RGB565_BLUE);
  }
  else if (knots > 1.00 && knots <= 12.00){
    screen.setTextColor(COLOR_RGB565_GREEN);
  }
  else if (knots > 12.00 && knots <= 19.00){
    screen.setTextColor(COLOR_RGB565_ORANGE);
  }
  else {
    screen.setTextColor(COLOR_RGB565_RED);
  }
  screen.print(knots);
  //if (knots >= cruisesetspeed+4.00){//reset cruise if current speed goes above cruisesetspeed + 4
  //  cruiseon = false;
  //}
  
  //temperature
  screen.setTextSize(2);
  screen.setCursor(12,215);
  screen.fillRect(11,214,55,17,erasecolor);
  screen.setTextColor(COLOR_RGB565_GREEN);
  screen.print(temperature,1);//temperature with only one decimal
  screen.drawCircle(64,209,2,COLOR_RGB565_GREEN);//degrees symbol

  //humidity
  screen.setTextSize(2);
  screen.setCursor(270,215);
  screen.fillRect(269,214,40,17,erasecolor);
  screen.setTextColor(COLOR_RGB565_GREEN);
  screen.print(humidity,0);//humidity with no decimals
  screen.print("%");//percentage symbol
  
  //battery
  screen.setTextSize(2);
  screen.setCursor(140,215);
  screen.fillRect(139,214,65,17,erasecolor);
  float batpercent = map(batteryvolts*10,300,420,0,100); //map battery voltage to a percentage value of 0 - 100
  if (batpercent > 80.0){
    screen.setTextColor(COLOR_RGB565_GREEN);
  }
  else if (batpercent <= 80.0 && batpercent > 50.0){
    screen.setTextColor(COLOR_RGB565_DGREEN);
  }
  else if (batpercent <= 50.0 && batpercent > 25.0){
    screen.setTextColor(COLOR_RGB565_YELLOW);
  }
  else if (batpercent <= 25.0 && batpercent > 12.0){
    screen.setTextColor(COLOR_RGB565_ORANGE);
  }
  else if (batpercent <= 12.0 && batpercent > 5.0){
    screen.setTextColor(COLOR_RGB565_RED);
  }
  else {
    screen.setTextColor(COLOR_RGB565_RED);
    batterywarning();
  }
  screen.print(batpercent,0);//battery percentage with no decimals
  screen.print("%");//percentage symbol

  //dist to empty; for 36V 5AH (180W) that would be (at 20mph) 8 miles or 0.4 hours
  float batwatts = 0;
  #ifdef batterywattage
    batwatts = batterywattage;
  #else
    batwatts = batteryvoltage * batteryamps;
  #endif
  float bathours = batwatts / motorwattage;
  float batmiles = maxcruisespeed * bathours;
  float batmilesleft = map(batteryvolts*10,300,420,0,batmiles*100);//map function cant map floating values
  batmilesleft = batmilesleft / 100;
  float bathoursleft = map(batteryvolts*10,300,420,0,bathours*100);//map function cant map floating values
  bathoursleft = bathoursleft / 100;
  float batminutesleft = map(bathoursleft*100,0,100,0,600);//map function cant map floating values
  batminutesleft = batminutesleft / 10;
  screen.setTextSize(3);
  screen.setCursor(75,187);
  screen.fillRect(74,186,198,24,erasecolor);
  screen.setTextColor(COLOR_RGB565_BLUE);
  screen.print(abs(batmilesleft),1);//miles with no decimals
  screen.print("m|");//miles symbol
  screen.print(abs(batminutesleft),1);//hours with two decimals
  screen.print("m");
  
  //Angle
  screen.setTextSize(2);
  screen.setCursor(140,95);
  screen.fillRect(139,94,65,17,erasecolor);
  screen.setTextColor(COLOR_RGB565_GREEN);
  screen.print(gyro.y*100,1);//temperature with only one decimal
}

void handleFastDisplay(){
  //Angle
  screen.setTextSize(2);
  screen.setCursor(140,95);
  screen.fillRect(139,94,75,17,erasecolor);
  screen.setTextColor(COLOR_RGB565_GREEN);
  screen.print((baseyangle*100)-(gyro.y*100),1);//angle with only one decimal
}

void handleDisplayButtons(){
  //e-brake button
  if (ebrakeon != lastebrakeon){
    if (!ebrakeon){
      screen.fillRoundRect(220,45,90,60,12,erasecolor);//erase
      screen.drawRoundRect(225,50,80,50,12,backgroundcolor);
      screen.setTextSize(2);
      screen.setCursor(235,70);
      screen.setTextColor(COLOR_RGB565_RED);
      screen.print("E OFF");
    }
    else {
      screen.fillRoundRect(225,50,80,50,12,backgroundcolor);
      screen.setTextSize(2);
      screen.setCursor(235,70);
      screen.setTextColor(COLOR_RGB565_GREEN);
      screen.print("E ON");
    }
    lastebrakeon = ebrakeon;
  }
  if (!lockbtnon){
    screen.fillRoundRect(12,45,90,60,12,erasecolor);//erase
    screen.drawRoundRect(15,50,80,50,12,backgroundcolor);
    screen.setTextSize(1);
    screen.setCursor(25,70);
    screen.setTextColor(COLOR_RGB565_GREEN);
    screen.print("Unlocked");
  }
  else if (lockbtnon || locked){
    screen.fillRoundRect(15,50,80,50,12,backgroundcolor);
    screen.setTextSize(2);
    screen.setCursor(20,70);
    screen.setTextColor(COLOR_RGB565_RED);
    screen.print("Locked");
    ebrakeon = true;
    locked = true;
  }
  if (cruiseon != lastcruiseon){
    if (!cruiseon){
      screen.fillRoundRect(220,105,90,60,12,erasecolor);//erase
      screen.drawRoundRect(225,110,80,50,12,backgroundcolor);
      screen.setTextSize(2);
      screen.setCursor(235,130);
      screen.setTextColor(COLOR_RGB565_GREEN);
      screen.print("C OFF");
    }
    else {
      screen.fillRoundRect(225,110,80,50,12,backgroundcolor);
      screen.setTextSize(2);
      screen.setCursor(235,130);
      screen.setTextColor(COLOR_RGB565_ORANGE);
      screen.print("C ON");
    }
    lastcruiseon = cruiseon;
  }
}

void handleAngleLine(float angle1){  
  int x=160,y=140; //center point of the two lines

  screen.drawLine(x, y, previousotherx1, previousothery1, erasecolor);//line going right
  screen.drawLine(x+1, y+1, previousotherx1+1, previousothery1+1, erasecolor);//double thick line
  screen.drawLine(x, y, previousotherx2, previousothery2, erasecolor);//line going left
  screen.drawLine(x+1, y+1, previousotherx2+1, previousothery2+1, erasecolor);//double thick line

  #define DEG2RAD 0.0174532925
  float screenangle1 = 135; //value for right line to make it level
  float screenangle2 = 315; //value for left line to make it level; appears to be one line instead of two separate ones
  float angle2 = angle1;  
  int w=100,h=100; //length of each line; one going right and one going left
 
  #define linecolor COLOR_RGB565_NAVY
  float cosa1 = cos((screenangle1+angle1) * DEG2RAD), sina1 = sin((screenangle1+angle1) * DEG2RAD);
  int otherx1 = x - ((w * cosa1 / 2) - (h * sina1 / 2));
  int othery1 = y - ((h * cosa1 / 2) + (w * sina1 / 2));
  screen.drawLine(x, y, otherx1, othery1, linecolor);//line going right
  screen.drawLine(x+1, y+1, otherx1+1, othery1+1, linecolor);//double thick line
  float cosa2 = cos((screenangle2+angle2) * DEG2RAD), sina2 = sin((screenangle2+angle2) * DEG2RAD);
  int otherx2 = x - ((w * cosa2 / 2) - (h * sina2 / 2));
  int othery2 = y - ((h * cosa2 / 2) + (w * sina2 / 2));
  screen.drawLine(x, y, otherx2, othery2, linecolor);//line going left
  screen.drawLine(x+1, y+1, otherx2+1, othery2+1, linecolor);//double thick line

  previousotherx1 = otherx1;
  previousothery1 = othery1;
  previousotherx2 = otherx2;
  previousothery2 = othery2;
}

void batterywarning(){

}

void changepassword(){
  screen.fillScreen(backcolor);
  screen.fillScreen(COLOR_RGB565_DGRAY);
  screen.fillRoundRect(0, 0, 320, 240, 15, backgroundcolor);
  screen.fillRoundRect(5, 5, 310, 230, 15, backcolor);
  screen.setTextColor(backgroundcolor);
  screen.setTextSize(3);
  screen.setCursor(30, 11);
  screen.print("Change PASS");
  screen.drawFastHLine(10, 35, 300, COLOR_RGB565_GREEN);
  screen.drawFastHLine(10, 36, 300, COLOR_RGB565_GREEN);
  screen.setTextColor(COLOR_RGB565_GREEN);
  screen.setTextSize(1);
  screen.setCursor(40, 100);
  screen.print("Do three gestures to set new password...");
  
  password[0] = 0;
  password[1] = 0;
  password[2] = 0;

  int gesturepasscount = 0;
  redo:
  if(gr10_30.getDataReady()){
    gestures = gr10_30.getGesturesState();
    if(gestures&GESTURE_UP){
      Serial.println("Up");
      password[gesturepasscount] = GESTURE_UP;
    }
    if(gestures&GESTURE_DOWN){
      Serial.println("Down");
      password[gesturepasscount] = GESTURE_DOWN;
    }
    if(gestures&GESTURE_LEFT){
      Serial.println("Left");
      password[gesturepasscount] = GESTURE_LEFT;
    }
    if(gestures&GESTURE_RIGHT){
      Serial.println("Right");
      password[gesturepasscount] = GESTURE_RIGHT;
    }
    if(gestures&GESTURE_FORWARD){
      password[gesturepasscount] = GESTURE_FORWARD;
    }
    if(gestures&GESTURE_BACKWARD){
      password[gesturepasscount] = GESTURE_BACKWARD;
    }
    if(gestures&GESTURE_CLOCKWISE){
      password[gesturepasscount] = GESTURE_CLOCKWISE;
    }
    if(gestures&GESTURE_COUNTERCLOCKWISE){
      password[gesturepasscount] = GESTURE_COUNTERCLOCKWISE;
    }
    if(gestures&GESTURE_WAVE){
      password[gesturepasscount] = GESTURE_WAVE;
    }
    if(gestures&GESTURE_HOVER){
      password[gesturepasscount] = GESTURE_HOVER;
    }
    if(gestures&GESTURE_CLOCKWISE_C){
      password[gesturepasscount] = GESTURE_CLOCKWISE_C;
    }
    if(gestures&GESTURE_COUNTERCLOCKWISE_C){
      password[gesturepasscount] = GESTURE_COUNTERCLOCKWISE_C;
    }
    EEPROM.write(gesturepasscount,password[gesturepasscount]);
    EEPROM.commit();
    gesturepasscount++;
    Serial.println("gesturepasscount: " + String(gesturepasscount));   
  }

  if (gesturepasscount != passwordlength){
    goto redo;
  }
  Serial.println("Finished!");   
  gesturepasscount = 0;
  startDisplay();
  
}

void readThrottle(){
  throttleval = map(analogRead(throttlepin),throttleoffvalue,throttleonvalue,4095,0);
  Serial.println("Throttle: " + String(throttleval));
}

void readBattery(){
  if ((millis() - lastbatreading) > readbatteryint){
    if (curbatread < batteryreadings){
      batsum = batsum + analogRead(batterypin);
      curbatread++;
    }
    else{
      float batteryread = batsum/batteryreadings;
      float batpinvolt = (batteryread*3.3)/4095;//3.3V and 4095 is the max resolution of ESP32
      if (testing){
        batpinvolt = 2.2;
      }
      batteryvolts = batpinvolt*16;//(1000/(15000+1000)); //(R2/(R1+R2));//VOUT = VIN*R2/R1+R2 ; classic equation for voltage dividers
      
      curbatread = 0;
    }
    lastbatreading = millis();
  }
}

void handleGestures(){
  if(gr10_30.getDataReady()){
    gestures = gr10_30.getGesturesState();
    if(gestures&GESTURE_UP){
      Serial.println("Up");
      cruiseon = true;
    }
    if(gestures&GESTURE_DOWN){
      Serial.println("Down");
      cruiseon = false;
    }
    if(gestures&GESTURE_LEFT){
      Serial.println("Left");
      ebrakeon = true;
    }
    if(gestures&GESTURE_RIGHT){
      Serial.println("Right");
      ebrakeon = false;
    }
    if(gestures&GESTURE_FORWARD){
      Serial.println("Forward");
    }
    if(gestures&GESTURE_BACKWARD){
      Serial.println("Backward");
    }
    if(gestures&GESTURE_CLOCKWISE){
      Serial.println("Clockwise");
    }
    if(gestures&GESTURE_COUNTERCLOCKWISE){
      Serial.println("Contrarotate");
    }
    if(gestures&GESTURE_WAVE){
      Serial.println("Wave");
    }
    if(gestures&GESTURE_HOVER){
      Serial.println("Hover");
    }
    if(gestures&GESTURE_CLOCKWISE_C){
      Serial.println("Continuous clockwise");
    }
    if(gestures&GESTURE_COUNTERCLOCKWISE_C){
      Serial.println("Continuous counterclockwise");
      screen.fillRoundRect(15,50,80,50,12,backgroundcolor);
      screen.setTextSize(2);
      screen.setCursor(20,70);
      screen.setTextColor(COLOR_RGB565_RED);
      screen.print("Locked");
      screen.fillRoundRect(225,50,80,50,12,backgroundcolor);
      screen.setTextSize(2);
      screen.setCursor(235,70);
      screen.setTextColor(COLOR_RGB565_GREEN);
      screen.print("E ON");
      ebrakeon = true;
      locked = true;
    }
  }
}

void lockedloop(){
  if(gr10_30.getDataReady()){
    gestures = gr10_30.getGesturesState();
    if (gestures&password[passc]){
      Serial.println("Correct!");
      passc++;
      Serial.println("PASSC: " + String(passc));
    }
    else {
      Serial.println("Wrong");
      passc=0;
    }
  }
  if (passc >= passwordlength){
    Serial.println("Unlocked!");
    passc=0;
    locked = false;
  }
}

void readGNSS(){
  utc = gnss.getUTC();
  date = gnss.getDate();
  latitude = gnss.getLat();
  longitude = gnss.getLon();
  altitude = gnss.getAlt();
  satelites = gnss.getNumSatUsed();
  knots = gnss.getSog();
  course = gnss.getCog();
}

void readAccelGyro(){
  int i = 0;
  int rslt;
  int16_t accelGyro[6]={0}; 
  
  //get both accel and gyro data from bmi160
  //parameter accelGyro is the pointer to store the data
  rslt = bmi160.getAccelGyroData(accelGyro);
  if(rslt == 0){
    for(i=0;i<6;i++){
      if (i<3){
        //the first three are gyro data
        //Serial.print(accelGyro[i]*3.14/180.0);Serial.print("\t");
        if (i==0){accel.x = accelGyro[i]*3.14/180.0;}
        else if (i==1){accel.y = accelGyro[i]*3.14/180.0;}
        else if (i==2){accel.z = accelGyro[i]*3.14/180.0;}
      }else{
        //the following three data are accel data
        //Serial.print(accelGyro[i]/16384.0);Serial.print("\t");
        if (i==3){gyro.x = accelGyro[i]/16384.0;}
        else if (i==4){gyro.y = accelGyro[i]/16384.0;}
        else if (i==5){gyro.z = accelGyro[i]/16384.0;}
      }
    }
    //Serial.println();
  }else{
    Serial.println("err");
  }
}

void readTempHum(){
  if(aht20.startMeasurementReady(/* crcEn = */true)){
    temperature = aht20.getTemperature_F();
    humidity = aht20.getHumidity_RH();
  }
}

void handleMotordriver(){
  digitalWrite(cruisepin,cruiseon);
  digitalWrite(brakepin,ebrakeon);
  ledcWrite(speedChannel, map(throttleval,400,4095,0,255));//pwm signal for speed
}

void handleLeds(){
  if ((millis() - lastcoursereadtime) > coursechangetime){//handle turning signal
    float coursechange = 0;
    coursechange = course - lastcoursereading;
    if (abs(coursechange) > courseturnsignalinc){
      if (coursechange < 0){
        leftblinker();
      }
      else {
        rightblinker();
      }
    }
    lastcoursereading = course; 
    lastcoursereadtime = millis();   
  }
  if ((millis() - lastspeedreadtime) > speedchangetime){//handle brake light
    float speeddiferrence = 0;
    speeddiferrence = knots - lastspeedreading;
    if (speeddiferrence < speedbrakeinc){
      brakelight(true);
    }
    else {
      brakelight(false);
    }
    lastspeedreading = knots;
    lastspeedreadtime = millis();
  }
}

void leftblinker(){
  if (animationcount == 1){
    if ((millis() - startmillis) < 300){
      pixels.setPixelColor(leftturn3, pixels.Color(255, 255, 0));
      pixels.show();
    }
    else {
      animationcount++;
      startmillis = millis();
    }
  }
  else if (animationcount == 2){
    if ((millis() - startmillis) < 300){
      pixels.setPixelColor(leftturn3, pixels.Color(0, 0, 0));
      pixels.setPixelColor(leftturn2, pixels.Color(255, 255, 0));
      pixels.show();
    }
    else {
      animationcount++;
      startmillis = millis();
    }
  }
  if (animationcount == 3){
    if ((millis() - startmillis) < 2000){
      pixels.setPixelColor(leftturn2, pixels.Color(0, 0, 0));
      pixels.setPixelColor(leftturn1, pixels.Color(255, 255, 0));
      pixels.show();
    }
    else {
      animationcount++;
      startmillis = millis();
    }
  }
  if (animationcount == 4){
    if ((millis() - startmillis) < 800){
      pixels.setPixelColor(leftturn1, pixels.Color(0, 0, 0));
      pixels.show();
    }
    else {
      animationcount = 1;
      startmillis = millis();
    }
  }
}

void rightblinker(){
  if (animationcount == 1){
    if ((millis() - startmillis) < 300){
      pixels.setPixelColor(rightturn3, pixels.Color(255, 255, 0));
      pixels.show();
    }
    else {
      animationcount++;
      startmillis = millis();
    }
  }
  else if (animationcount == 2){
    if ((millis() - startmillis) < 300){
      pixels.setPixelColor(rightturn3, pixels.Color(0, 0, 0));
      pixels.setPixelColor(rightturn2, pixels.Color(255, 255, 0));
      pixels.show();
    }
    else {
      animationcount++;
      startmillis = millis();
    }
  }
  if (animationcount == 3){
    if ((millis() - startmillis) < 2000){
      pixels.setPixelColor(rightturn2, pixels.Color(0, 0, 0));
      pixels.setPixelColor(rightturn1, pixels.Color(255, 255, 0));
      pixels.show();
    }
    else {
      animationcount++;
      startmillis = millis();
    }
  }
  if (animationcount == 4){
    if ((millis() - startmillis) < 800){
      pixels.setPixelColor(rightturn1, pixels.Color(0, 0, 0));
      pixels.show();
    }
    else {
      animationcount = 1;
      startmillis = millis();
    }
  }
}

void brakelight(bool brakelighton){
  if (brakelighton){
    for (int i=0;i<4;i++){
      pixels.setPixelColor(i+16, pixels.Color(255, 0, 0));//brake light on
    }
  }
  else {
    for (int i=0;i<4;i++){
      pixels.setPixelColor(i+16, pixels.Color(115, 0, 0));//dim brake light
    }
  }
}

void startDisplay(){
  screen.fillScreen(backcolor);
  screen.setTextWrap(false);

  screen.fillScreen(COLOR_RGB565_DGRAY);
  screen.fillRoundRect(0, 0, 320, 240, 15, backgroundcolor);
  screen.fillRoundRect(5, 5, 310, 230, 15, backcolor);
  
  //title
  screen.setTextColor(backgroundcolor);
  screen.setTextSize(3);
  screen.setCursor(30, 11);
  screen.print("DIY E-Scooter");

  screen.drawFastHLine(10, 35, 300, COLOR_RGB565_GREEN);
  screen.drawFastHLine(10, 36, 300, COLOR_RGB565_GREEN);

  //speed label
  screen.setTextColor(COLOR_RGB565_YELLOW);
  screen.setTextSize(2);
  screen.setCursor(125,40);
  screen.print("Speed");

  //dist and time to empty labels
  screen.setTextColor(COLOR_RGB565_YELLOW);
  screen.setTextSize(2);
  screen.setCursor(91,170);
  screen.print("Dist   Time");

  //boost button
  screen.drawRoundRect(225,50,80,50,12,backgroundcolor);
  screen.setTextSize(2);
  screen.setCursor(235,70);
  screen.setTextColor(COLOR_RGB565_RED);
  screen.print("E OFF");

  //lock button
  screen.drawRoundRect(15,50,80,50,12,backgroundcolor);
  screen.setTextSize(1);
  screen.setCursor(25,70);
  screen.setTextColor(COLOR_RGB565_GREEN);
  screen.print("Unlocked");

  //cruise button
  screen.drawRoundRect(225,110,80,50,12,backgroundcolor);
  screen.setTextSize(2);
  screen.setCursor(235,130);
  screen.setTextColor(COLOR_RGB565_GREEN);
  screen.print("C OFF");
}

void startGestureSensor(){

  if(gr10_30.begin() != 0){
    Serial.println(" Sensor initialize failed!!");
    delay(1000);
  }
  /** Set the gesture to be enabled
 *  GESTURE_UP
 *  GESTURE_DOWN
 *  GESTURE_LEFT
 *  GESTURE_RIGHT
 *  GESTURE_FORWARD
 *  GESTURE_BACKWARD
 *  GESTURE_CLOCKWISE
 *  GESTURE_COUNTERCLOCKWISE
 *  GESTURE_WAVE              It is not suggested to enable rotation gesture (CW/CCW) and wave gesture at the same time.
 *  GESTURE_HOVER             Disable other gestures when hover gesture enables.
 *  GESTURE_UNKNOWN
 *  GESTURE_CLOCKWISE_C
 *  GESTURE_COUNTERCLOCKWISE_C
 */
  gr10_30.enGestures(GESTURE_UP|GESTURE_DOWN|GESTURE_LEFT|GESTURE_RIGHT|GESTURE_FORWARD|GESTURE_BACKWARD|GESTURE_CLOCKWISE|GESTURE_COUNTERCLOCKWISE|GESTURE_CLOCKWISE_C|GESTURE_COUNTERCLOCKWISE_C);
  /**
 * Set the detection window you want, only data collected in the range are valid
 * The largest window is 31, the configured number represents distance from the center to the top, bottom, left and right
 * For example, if the configured distance from top to bottom is 30, then the distance from center to top is 15, and distance from center to bottom is also 15
 * udSize Range of distance from top to bottom      0-31
 * lrSize Range of distance from left to right      0-31
 */
  gr10_30.setUdlrWin(30, 30);//0-31,0-31
  /**
 * Set moving distance that can be recognized as a gesture
 * Distance range 5-25, must be less than distances of the detection window
 */
  gr10_30.setLeftRange(10);
  gr10_30.setRightRange(10);
  gr10_30.setUpRange(10);
  gr10_30.setDownRange(10);
  gr10_30.setForwardRange(10);
  gr10_30.setBackwardRange(10);
  /**
 * Set distance of moving forward and backward that can be recognized as a gesture
 * Distance range 1-15
 */
  gr10_30.setForwardRange(10);
  gr10_30.setBackwardRange(10);
/**
 * Set rotation angle that can trigger the gesture
 * count Default is 16 range 0-31
 * count Rotation angle is 22.5 * count
 * count = 16 22.5*count = 360  Rotate 360° to trigger the gesture
 */
  gr10_30.setCwsAngle(/*count*/16);
  gr10_30.setCcwAngle(/*count*/16);
  /**
 * Set degrees of continuous rotation that can trigger the gesture
 * count Default is 4  range 0-31
 * count The degrees of continuous rotation is 22.5 * count
 * For example: count = 4 22.5*count = 90
 * Trigger the clockwise/counterclockwise rotation gesture first, if keep rotating, then the continuous rotation gesture will be triggered once every 90 degrees
 */
  gr10_30.setCwsAngleCount(/*count*/8);
  gr10_30.setCcwAngleCount(/*count*/8);
}

void interruptRoutine(){
  Serial.println("Interrupt...");
}
