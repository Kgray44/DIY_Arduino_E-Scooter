#include <Adafruit_NeoPixel.h>

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

Adafruit_NeoPixel pixels(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

void start_leds(){
  pixels.begin();
  pixels.clear();
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

