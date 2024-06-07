#include <SPI.h>
#include "DFRobot_GNSS.h"
#include "DFRobot_BMI160.h"
#include "DFRobot_AHT20.h"
#include "DFRobot_GR10_30.h"
#include <EEPROM.h>
#include "settings.h"
#include "leds.h"
#include "allwifi.h"

//uncomment to set EEPROM to default values.
//#define EEPROM_RESET

//uncomment to have scooter reboot where it can be programmed again
//#define CONTINUE_PROGRAMMING

//Testing
bool testing = false;


DFRobot_GNSS_I2C gnss(&Wire,GNSS_DEVICE_ADDR);
DFRobot_BMI160 bmi160;
const int8_t i2c_addr = 0x69;
DFRobot_AHT20 aht20;
DFRobot_GR10_30 gr10_30(/*addr = */GR10_30_DEVICE_ADDR, /*pWire = */&Wire);

void setup() {
  digitalWrite(speedpin, LOW);
  //pinMode(rpmpin, INPUT);

  Serial.begin(115200);

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  for (int thisReading2 = 0; thisReading2 < numReadings2; thisReading2++) {
    readings2[thisReading2] = 0;
  }

  EEPROM.begin(EEPROM_SIZE+2);


  #ifdef EEPROM_RESET
  EEPROM.write(0, (1<<0));
  EEPROM.write(1, (1<<1));
  EEPROM.write(2, (1<<2));
  EEPROM.write(3, 100);
  EEPROM.write(4, 0);
  EEPROM.commit();
  Serial.println();
  Serial.print(EEPROM_SIZE);
  Serial.println(" bytes written on Flash . Values are:");
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    Serial.print(byte(EEPROM.read(i))); Serial.print(" ");
  }
  Serial.println(); Serial.println("----------------------------------");
  #endif


  for (int i=0;i<passwordlength;i++){//read password from EEPROM
    password[i] = EEPROM.read(i);    
  }
  uppervalue = EEPROM.read(3);
  #ifdef CONTINUE_PROGRAMMING
  wakeup_status = 2;
  #else
  wakeup_status = EEPROM.read(4);
  #endif

  pinMode(batterypin,    INPUT);
  pinMode(throttlepin,   INPUT);
  pinMode(brakepin,     OUTPUT);
  pinMode(cruisepin,    OUTPUT);
  pinMode(speedpin,     OUTPUT);
  pinMode(brakeswitchpin,INPUT);
  //pinMode(lowspeedpin,  OUTPUT);
  //pinMode(highspeedpin, OUTPUT);
  digitalWrite(speedpin, LOW);
  digitalWrite(brakepin, LOW);
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

  uint8_t status;
  if((status = aht20.begin()) != 0){
    Serial.print("AHT20 sensor initialization failed. error status : ");
    Serial.println(status);
    delay(1000);
  }
  
  startGestureSensor();
  ts.begin();
  ts.setRotation(3);
  
  screen.begin();
  screen.setRotation(3);

  startDisplay();

  Serial.println(password[0]);
  Serial.println(password[1]);
  Serial.println(password[2]);

  if (wakeup_status==1){handleWiFi(false);}
  else if (wakeup_status==2){handleWiFi(true);}
}

void loop() {
  if ((millis() - lastreadgnss) > readgnssinc){
    saveprevreadGNSS();
    readGNSS();
    lastreadgnss = millis();
  }
  /*if (!ledsoff){
    handleLeds();
  }
  else {
    pixels.clear();
    pixels.show();  
  }*/
  if (locked){
    lockedloop();
    if (satelites > 8){
      if (knots > 2){
        //ifttt_counter++;
      }
    }
    return;
  }
  if (ts.touched()){
    TS_Point p = ts.getPoint();
    handleTouch(abs(map(p.x,touchxmax,touchxmin,0,320)),abs(map(p.y,touchymax,touchymin,0,240)));
    handleDisplayButtons();
  }
  //readAccelGyro();
  //readGyro();
  readTempHum();
  //readCurrent();
  readBattery();
  readThrottle();

  if (digitalRead(brakeswitchpin) != lastswitchvalue){
    lastswitchvalue = digitalRead(brakeswitchpin);
    //digitalWrite(brakepin, digitalRead(brakeswitchpin));
    /*if (lastswitchvalue==1){digitalWrite(highspeedpin, HIGH);digitalWrite(lowspeedpin, LOW);}
    else if (lastswitchvalue==0){digitalWrite(highspeedpin, LOW);digitalWrite(lowspeedpin, HIGH);}*/
    throttleval = 0;
    Serial.print("Speed toggle switched... status:");
    Serial.println(lastswitchvalue);
  }

  //if (!rightturnsignal && !leftturnsignal){
  //  if (lastturnstat){
  //    startDisplay();
  //    lastturnstat = false;
  //  }
    if (((millis() - lastdisplay) > readdisplayinc) && (satelites != 0)){
      handleSlowDisplay();
      lastdisplay = millis();
    }
    handleFastDisplay();
    //float handleAngleLineValue = (baseyangle*100) - (gyroy*100);
    //handleAngleLine(handleAngleLineValue);

  //}
  /*else {
    if (rightturnsignal){
      screen.fillRoundRect(15,38,290,195,12,backgroundcolor);
      screen.setTextSize(14);
      screen.setCursor(100,75);
      screen.setTextColor(COLOR_RGB565_RED);
      screen.print(">");
    }
    else if (leftturnsignal){
      screen.fillRoundRect(15,38,290,195,12,backgroundcolor);
      screen.setTextSize(14);
      screen.setCursor(100,75);
      screen.setTextColor(COLOR_RGB565_RED);
      screen.print("<");
    }
  }*/
  handleGestures();
  handleMotordriver();
  //handleSpeedLimit();
}

void handleTouch(int x,int y){
  //Serial.print("Touch X = " + String(x));
  //Serial.println(" | Touch Y = " + String(y));

  if (x >= 215 && x <= 320){
    if (y < 45){
      changepassword();
    }
    else if (y >= 45 && y <= 104){
      if ((millis() - lastebrakechange) > 1000){//wait 1 second between reading button as pressed
        //handleWiFi();
        handleSpeedSlider();
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
    else if (y >= 105 && y <= 160){
      handleWiFi(false);
    }  
  }
}

bool buttonpressed = false;
int touchx=0,touchy=0,valt=0,lastx=0;
void handleSpeedSlider(){
  //clear
  screen.fillRoundRect(15,38,290,195,12,backgroundcolor);
  screen.drawRoundRect(25,90,270,50,20,COLOR_RGB565_GREEN);
  screen.drawFastHLine(20,115,280,COLOR_RGB565_GREEN);

  //button
  screen.fillRoundRect(115,160,90,60,12,erasecolor);//erase
  screen.drawRoundRect(120,165,80,50,12,COLOR_RGB565_CYAN);
  screen.setTextSize(2);
  screen.setCursor(140,185);
  screen.setTextColor(COLOR_RGB565_GREEN);
  screen.print("SET");

  //slider
  screen.fillRoundRect(map(uppervalue,0,100,25,295),80,6,70,3,COLOR_RGB565_RED);

  while (true){
    yield();
    lastx = touchx;
    if (ts.touched()){
      TS_Point p = ts.getPoint();
      touchx = abs(map(p.x,touchxmax,touchxmin,0,320));
      touchy = abs(map(p.y,touchymax,touchymin,0,240));
    }
    if (lastx != touchx){
      screen.fillRoundRect(lastx-3,80,6,70,3,backgroundcolor);
      //slider
      screen.drawRoundRect(25,90,270,50,20,COLOR_RGB565_GREEN);
      screen.drawFastHLine(20,115,280,COLOR_RGB565_GREEN);

      screen.fillRoundRect(40,160,35,45,12,erasecolor);//erase
      screen.setCursor(45,165);
      screen.setTextColor(COLOR_RGB565_RED);
      int va = map(valt,25,295,0,100);
      va = constrain(va,0,100);
      screen.print(va);
    }
    //slider
    if (touchy >= 90 && touchy <= 140){
      if (touchx >= 25 && touchx <= 295){
        screen.fillRoundRect(touchx-3,80,6,70,3,COLOR_RGB565_RED);
        valt = map(touchx-3,29,290,25,295);
      }
    }
    //button
    else if (touchy >= 165 && touchy <= 215){
      if (touchx >= 120 && touchx <= 200){
        screen.fillRoundRect(115,160,90,60,12,erasecolor);//erase
        screen.fillRoundRect(120,165,80,50,12,COLOR_RGB565_GREEN);
        screen.setTextSize(2);
        screen.setCursor(145,185);
        screen.setTextColor(COLOR_RGB565_ORANGE);
        screen.print("OK");
        uppervalue = map(valt,25,295,0,100);
        uppervalue = constrain(uppervalue,0,100);
        EEPROM.write(3,uppervalue);
        EEPROM.commit();
        delay(1000);
        touchx=0,touchy=0,lastx=0;
        startDisplay();
        return;
      }
    }

  }
}

void handleWiFi(bool instaap){
  int lastuppervalue = uppervalue;
  WiFiStart(instaap);
  //IPAddress myIP = WiFi.softAPIP();
  screen.fillRoundRect(15,38,290,195,12,backgroundcolor);
  screen.setTextSize(2);
  screen.setTextColor(COLOR_RGB565_RED);
  screen.setCursor(15,60);
  screen.print("Waiting for speed");//display unlock warning
  screen.setCursor(15,90);
  screen.print("Touch screen to");//display unlock warning
  screen.setCursor(15,110);
  screen.print("program on bootup");//display unlock warning
  screen.setCursor(15,145);
  screen.print("Reboot normal: ");//display unlock warning
  if (!wakeup_status){
    screen.setTextColor(COLOR_RGB565_GREEN);
    screen.setCursor(200,145);
    screen.print("True  ");//display unlock warning
  }
  else {
    screen.setTextColor(COLOR_RGB565_GREEN);
    screen.setCursor(200,145);
    screen.print("False ");//display unlock warning
  }
  delay(500);
  while (uppervalue == lastuppervalue){
    WiFiRun();yield();
    if (ts.touched()){
      wakeup_status = !wakeup_status;
      if (!wakeup_status){
        screen.setTextColor(backgroundcolor);
        screen.setCursor(200,145);
        screen.print("False ");//display unlock warning
        screen.setTextColor(COLOR_RGB565_GREEN);
        screen.setCursor(200,145);
        screen.print("True  ");//display unlock warning
      }
      else {
        screen.setTextColor(backgroundcolor);
        screen.setCursor(200,145);
        screen.print("True  ");//display unlock warning
        screen.setTextColor(COLOR_RGB565_GREEN);
        screen.setCursor(200,145);
        screen.print("False ");//display unlock warning
      }
      delay(500);
    }
  }
}

void handleSlowDisplay(){
  //satelites
  screen.setTextSize(2);
  screen.setCursor(287,15);
  //screen.fillRect(286, 13, 26, 18, erasecolor);//backcolor
  if (justsaved){screen.setTextColor(erasecolor);screen.print(lastsatelites);screen.setCursor(287,15);}
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

  if (lastknots >= 10){
    screen.setCursor(100,62);
    if (justsaved){screen.setTextColor(erasecolor);screen.print(lastknots);screen.setCursor(100,62);}
  }
  else {
    screen.setCursor(110,62);//118,62
    if (justsaved){screen.setTextColor(erasecolor);screen.print(lastknots);screen.setCursor(110,62);}
  }
  if (knots >= 10){
    screen.setCursor(100,62);//118,62
    //screen.fillRect(92, 58, 132, 37, erasecolor);//117, 57, 90, 30,
  }
  else {
    screen.setCursor(110,62);//118,62
    //screen.fillRect(100, 58, 105, 37, erasecolor);//117, 57, 90, 30,
  }

  if (more_visible){
    screen.setTextColor(COLOR_RGB565_BLACK);
  }

  else {
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
  }
  screen.print(knots);
  //if (knots >= cruisesetspeed+4.00){//reset cruise if current speed goes above cruisesetspeed + 4
  //  cruiseon = false;
  //}
  
  //wattage...coming soon
  //screen.fillRoundRect(12,105,90,60,12,erasecolor);//erase
  
  //screen.drawRoundRect(15,110,80,50,12,backgroundcolor);
  screen.setTextSize(2);
  screen.setCursor(25,130);
  if (justsaved){screen.setTextColor(erasecolor);screen.print(lastaltitude);screen.setCursor(25,130);}
  screen.setTextColor(COLOR_RGB565_RED);
  screen.print(altitude,0);

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
  Serial.print("Battery: ");
  Serial.println(batteryvolts);
  //Serial.print("Current: ");
  //Serial.println(current);
  justsaved = false;
}

void handleFastDisplay(){
  //battery
  screen.setTextSize(2);
  screen.setCursor(110,215);
  screen.fillRect(109,214,65,17,erasecolor);
  float batpercent = map(batteryvolts*10,300,420,0,100); //map battery voltage to a percentage value of 0 - 100
  //batpercent = map(batpercent,40,100,0,100);
  batpercent = constrain(batpercent,0,100);

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

  screen.setTextSize(2);
  screen.setCursor(170,215);
  screen.fillRect(169,214,65,17,erasecolor);
  int vale = map(motorvalue,0,255,0,18);
  adjbat = batpercent+vale;
  adjbat = constrain(adjbat,0,100);
  if (adjbat > 80.0){
    screen.setTextColor(COLOR_RGB565_GREEN);
  }
  else if (adjbat <= 80.0 && adjbat > 50.0){
    screen.setTextColor(COLOR_RGB565_DGREEN);
  }
  else if (adjbat <= 50.0 && adjbat > 25.0){
    screen.setTextColor(COLOR_RGB565_YELLOW);
  }
  else if (adjbat <= 25.0 && adjbat > 12.0){
    screen.setTextColor(COLOR_RGB565_ORANGE);
  }
  else if (adjbat <= 12.0 && adjbat > 5.0){
    screen.setTextColor(COLOR_RGB565_RED);
  }
  else {
    screen.setTextColor(COLOR_RGB565_RED);
    batterywarning();
  }
  screen.print(adjbat,0);//adjusted battery percentage with no decimals
  screen.print("%");//percentage symbol

  //Angle
  screen.setTextSize(2);
  screen.setCursor(140,95);
  screen.fillRect(139,94,75,17,erasecolor);
  //if (justsaved){screen.setTextColor(erasecolor);screen.print(lastcourse);screen.setCursor(140,95);}
  screen.setTextColor(COLOR_RGB565_GREEN);
  screen.print(course);
  //screen.print((baseyangle*100)-(gyro.y*100),1);//angle with only one decimal
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
      screen.print("HIGH ");
    }
    else {
      screen.fillRoundRect(225,50,80,50,12,backgroundcolor);
      screen.setTextSize(2);
      screen.setCursor(235,70);
      screen.setTextColor(COLOR_RGB565_GREEN);
      screen.print(" LOW ");
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
      //Serial.println("Up");
      password[gesturepasscount] = GESTURE_UP;
    }
    if(gestures&GESTURE_DOWN){
      //Serial.println("Down");
      password[gesturepasscount] = GESTURE_DOWN;
    }
    if(gestures&GESTURE_LEFT){
      //Serial.println("Left");
      password[gesturepasscount] = GESTURE_LEFT;
    }
    if(gestures&GESTURE_RIGHT){
      //Serial.println("Right");
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
    //Serial.println("gesturepasscount: " + String(gesturepasscount));   
  }

  if (gesturepasscount != passwordlength){
    goto redo;
  }
  //Serial.println("Finished!");   
  gesturepasscount = 0;
  startDisplay();
  
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
      batteryvolts = batpinvolt*16.55;//(1000/(15000+1000)); //(R2/(R1+R2));//VOUT = VIN*R2/R1+R2 ; classic equation for voltage dividers
      curbatread = 0;
      batsum = 0;

    }
    lastbatreading = millis();
  }
}

void readCurrent(){
  if ((millis() - lastcurreading) > readcurrentint){
    if (curcurread < currentreadings){
      cursum = cursum + analogRead(currentpin);
      curcurread++;
    }
    else{
      float currentread = cursum/currentreadings;
      float curpinvolt = (currentread*3.3)/4095;//3.3V and 4095 is the max resolution of ESP32
      current = (curpinvolt-1.65)/0.040;
      //current = constrain(current,0,50);
      watts = current*batteryvolts;
      //watts = constrain(watts,0,500);

      curcurread = 0;
      cursum = 0;
    }
    lastcurreading = millis();
  }
}

void readGyro(){
  /*for (curgyroread=0;curgyroread<gyroreadings;curgyroread++){
    gyrosum = gyrosum + gyro.y;
    delay(readgyroint);
  }
  gyroy = gyrosum/gyroreadings;      
  curgyroread = 0;
  gyrosum = 0;*/
  
  if ((millis() - lastgyroreading) > readgyroint){
    if (curgyroread < gyroreadings){
      gyrosum = gyrosum + gyro.y;
      curgyroread++;
    }
    else{
      gyroy = gyrosum/gyroreadings;      
      curgyroread = 0;
      gyrosum = 0;
    }
    lastgyroreading = millis();
  }
}

void handleGestures(){
  if(gr10_30.getDataReady()){
    gestures = gr10_30.getGesturesState();
    if(gestures&GESTURE_UP){
      //Serial.println("Up");
      cruiseon = true;
    }
    if(gestures&GESTURE_DOWN){
      //Serial.println("Down");
      //if (cruiseon){
      //  ledsoff = !ledsoff;
      //}
      cruiseon = false;
    }
    if(gestures&GESTURE_LEFT){
      more_visible = false;
      //Serial.println("Left");
      //ebrakeon = true;
      
      /*if (rightturnsignal){
        rightturnsignal = false;
        pixels.clear();
        pixels.show();
      }
      else {
        leftturnsignal = true;
        lastturnstat = true;
      }*/
    }
    if(gestures&GESTURE_RIGHT){
      more_visible = true;
      //Serial.println("Right");
      //ebrakeon = false;
      /*if (leftturnsignal){
        leftturnsignal = false;
        pixels.clear();
        pixels.show();
      }
      else {
        rightturnsignal = true;
        lastturnstat = true;
      }*/
    }
    if(gestures&GESTURE_FORWARD){
      //Serial.println("Forward");
    }
    if(gestures&GESTURE_BACKWARD){
      //Serial.println("Backward");
    }
    if(gestures&GESTURE_CLOCKWISE){
      //Serial.println("Clockwise");
    }
    if(gestures&GESTURE_COUNTERCLOCKWISE){
      //Serial.println("Contrarotate");
    }
    if(gestures&GESTURE_WAVE){
      
      //Serial.println("Wave");
    }
    if(gestures&GESTURE_HOVER){
      //Serial.println("Hover");
    }
    if(gestures&GESTURE_CLOCKWISE_C){
      //Serial.println("Continuous clockwise");
      //headlight = !headlight;
    }
    if(gestures&GESTURE_COUNTERCLOCKWISE_C){
      //Serial.println("Continuous counterclockwise");
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
    //Serial.print("Gesture: ");
    //Serial.println(gestures);
    if (gestures&password[passc]){
      //Serial.println("Correct!");
      //Serial.println(password[passc]);
      passc++;
      //Serial.println("PASSC: " + String(passc));
    }
    else {
      //Serial.println("Wrong");
      passc=0;
    }
  }
  if (passc >= passwordlength){
    //Serial.println("Unlocked!");
    passc=0;
    locked = false;
    //lock = false;
  }
}

void saveprevreadGNSS(){
  lastlongitude = longitude;
  lastlatitude = latitude;
  lastaltitude = altitude;
  lastsatelites = satelites;
  lastknots = knots;
  lastcourse = course;
  justsaved = true;
}

void readGNSS(){
  utc = gnss.getUTC();
  date = gnss.getDate();
  latitude = gnss.getLat();
  longitude = gnss.getLon();
  altitude = gnss.getAlt();
  satelites = gnss.getNumSatUsed();
  float lastsog = knots;
  //float sog = 
  knots = gnss.getSog();//knots
  knots *= 1.151;//mph conversion
  /*if ((sog - lastsog) > 5){
    knots = 0;
  }*/
  // subtract the last reading:
  /*total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = sog;
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;
  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }
  // calculate the average:
  knots = total / numReadings;*/
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
    //Serial.println("err");
  }
}

void readTempHum(){
  if(aht20.startMeasurementReady(/* crcEn = */true)){
    temperature = aht20.getTemperature_F();
    humidity = aht20.getHumidity_RH();
  }
  HI = 0.5 * (temperature + 61.0 + ((temperature-68.0)*1.2) + (humidity*0.094));
  if (HI > 80.0){
    HI = -42.379 + 2.04901523*temperature + 10.14333127*humidity - .22475541*temperature*humidity - .00683783*temperature*temperature - .05481717*humidity*humidity + .00122874*temperature*temperature*humidity + .00085282*temperature*humidity*humidity - .00000199*temperature*temperature*humidity*humidity;
    if ((temperature > 80.0 && temperature < 112.0) && (humidity < 13.0)){
      HI = ((13-humidity)/4)*sqrt((17-abs(temperature-95.0))/17);
    }
    else if ((temperature > 80.0 && temperature < 87.0) && (humidity > 85.0)){
      HI = ((humidity-85)/10) * ((87-temperature)/5);
    }
  }
  temperature = HI;
}

void handleMotordriver(){
  
  motorvalue = map(readThrottle(),300,3700,0,255);
  motorvalue = constrain(motorvalue,0,255);
  
  motorvalue = map(motorvalue,255,0,0,255);
  
  if (motorvalue >= 200 && motorvalue <= 245){
    if (abs(motorvalue - lastthrottleval) <= 5){
      motorvalue = lastthrottleval;
    }
    else {
      lastthrottleval = motorvalue;
    }
  }
  else if (motorvalue < 200 && motorvalue > 110){
    if (abs(motorvalue - lastthrottleval) <= 15){
      motorvalue = lastthrottleval;
    }
    else {
      lastthrottleval = motorvalue;
    }
  }
  else if (motorvalue <= 110 && motorvalue > 5){
    motorvalue = map(motorvalue,5,120,32,87);
  }
  if (motorvalue >= 245){motorvalue = 255;} else if (motorvalue <= 5){motorvalue = 0;}
  if ((lastthrottleval - motorvalue) <= -7){
    motorvalue = lastthrottleval - 8;
    lastthrottleval = motorvalue;
  }
  motorvalue = abs(motorvalue);
  
  motorvalue = constrain(motorvalue,0,255);
  if (throttleval == 0){
    motorvalue = 255;
  }
  motorvalue = map(motorvalue,255,0,0,(map(uppervalue,0,100,0,255)));//speed limit
  #ifdef output_conversion
  motorvalue = map(motorvalue,0,255,255,0);
  #endif

  if (uppervalue<95){
    if (knots > (13*(uppervalue*0.01))){//13 is the usual top speed.  70% throttle would be 9.1kn
      motorvalue = 0;
    }
  }
  //smoothing
  // subtract the last reading:
  total2 = total2 - readings2[readIndex2];
  // read from the sensor:
  readings2[readIndex2] = motorvalue;
  // add the reading to the total:
  total2 = total2 + readings2[readIndex2];
  // advance to the next position in the array:
  readIndex2 = readIndex2 + 1;

  // if we're at the end of the array...
  if (readIndex2 >= numReadings2) {
    // ...wrap around to the beginning:
    readIndex2 = 0;
  }

  // calculate the average:
  motorvalue = total2 / numReadings2;
  //Serial.print("Motor val: ");
  //Serial.print(motorvalue);  Serial.print(" | ");   Serial.println("Throttle: " + String(throttleval));
  
  ledcWrite(speedChannel, motorvalue);//pwm signal for speed
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
  screen.print("HIGH ");

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
    //Serial.println(" Sensor initialize failed!!");
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
  //Serial.println("Interrupt...");
}
