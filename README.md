# DIY_Arduino_E-Scooter
This repository is for my Hackster project on how to build an e-scooter with Arduino!

Tutorial can be found here:
https://www.hackster.io/k-gray/diy-arduino-firebeetle-e-scooter-6148d2

## NOTE

Remember to always use the latest code version!  Any previous versions may not work properly.  The newest version is "V1.2" found in releases.

## Explanation of files

The "DIY_E-Scooter Libraries.zip" file is a compressed file of all the libaries used in this project.  If using the Arduino Cloud, just import this one file in the libraries section, and you will have no issues at all!

The "diy_e-scooter" folder has the "diy_e-scooter.ino" sketch in it, which is the entire code for this project currently.  This code runs without any WiFi or wireless activity.  A new version is coming soon that includes WiFi using the Arduino Cloud!

The "diy_e-scooter_cloud" folder has the "diy_e-scooter_cloud.ino" sketch in it, which is the entire code for this project that includes the use of the Arduino Cloud!  The Cloud is used to lock / unlock the scooter remotely, check to see if its stolen, and set a speed limit for yourself!  This version also includes an updated version of the LEDs that work; this includes turning signals for left and right (that can be controlled with left and right gestures, and also come on automatically) and a brake light that comes on automatically if youre slowing down.

The diy_e-scooter_cloud_v3 includes better motor control, and the option to set the voltage conversion to on or off.

The diy_e-scooter_cloud_v4 includes some better additions to the WiFi (although I am not completely finished), and a better throttle control!  The starting screen now displays WiFi info.

# Updates
There will be more updates coming after further testing!  This is only the first version.

WS2812 LEDs for turning signals and braking lights have been completed!

Better WiFI updates coming soon...
