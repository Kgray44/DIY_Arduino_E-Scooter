# DIY_Arduino_E-Scooter
This repository is for my Hackster project on how to build an e-scooter with Arduino!

Tutorial can be found here:
https://www.hackster.io/k-gray/diy-arduino-firebeetle-e-scooter-6148d2

## NOTE

Remember to always use the latest code version!  Any previous versions may not work properly, although you can try to use them anyways if you'd like to.  The newest version is "V2.1" found in releases.

## Explanation of files

The "DIY_E-Scooter Libraries.zip" file is a compressed file of all the libaries used in this project.  If using the Arduino Cloud, just import this one file in the libraries section, and you will have no issues at all!

The "diy_e-scooter" folder has the "diy_e-scooter.ino" sketch in it, which is the entire code for this project currently.  This code runs without any WiFi or wireless activity.  A new version is coming soon that includes WiFi using the Arduino Cloud!

The "diy_e-scooter_cloud" folder has the "diy_e-scooter_cloud.ino" sketch in it, which is the entire code for this project that includes the use of the Arduino Cloud!  The Cloud is used to lock / unlock the scooter remotely, check to see if its stolen, and set a speed limit for yourself!  This version also includes an updated version of the LEDs that work; this includes turning signals for left and right (that can be controlled with left and right gestures, and also come on automatically) and a brake light that comes on automatically if youre slowing down.

The diy_e-scooter_cloud_v3 includes better motor control, and the option to set the voltage conversion to on or off.

The diy_e-scooter_cloud_v4 includes some better additions to the WiFi (although I am not completely finished), and a better throttle control!  The starting screen now displays WiFi info.

The diy_e-scooter_cloud_v5 includes the capability of adding a current sensor, along with separating the WiFi related code from the main code, and placing it in a seperate code tab!

### UPDATED (06/07/2024)

The Current/e-scooter is my exact code I currently use!  It includes better WiFi, better motor control, and better organization!

# Updates
There will be more updates coming after further testing!

WS2812 LEDs for turning signals and braking lights have been completed! (not setup in the "Current" code)

Throttle response improvements have been completed! (Even more improvements in the new update! 06/07/2024)

Better motor control has been completed! (Even more improvements in the new update! 06/07/2024)

Current sensor addition completed!

Seperate code tab for pin definitions completed! (Added several more tabs for better organization! 06/07/2024)

~~Better WiFi updates coming soon...~~

Better WiFi updates completed! (06/07/2024)
