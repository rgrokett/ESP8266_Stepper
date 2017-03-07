# ESP8266_Stepper
ESP8266 HUZZAH WiFi Operated Stepper Motor


Overview
Surprisingly, you can run a powerful stepper motor (via motor controller) from 
the Adafruit Huzzah ESP8266 WiFi module and thus control the stepper via 
browser or web service, turning your stepper into an Internet of Things (IoT) 
device. This makes for a compact and low-cost remotely controlled motor, good 
for robots, mechanical arms and more.

Requirements
I used the following Adafruit components for the Stepper Motor and ESP8266 
right out of the box. Otherwise, you would need to add regulators and 
related components to make the voltages compatible.


IMPORTANT NOTE! The power regulator on the Adafruit HUZZAH ESP8266 board 
has CHANGED. It no longer can handle 12v DC. A 5VDC power supply to feed 
the V+ and a 12v supply to the Stepper now required!


Older HUZZAH ESP8266's included a 3-16v regulator, so are unaffected.


##Hardware
 *  Adafruit HUZZAH ESP8266 Breakout  https://www.adafruit.com/products/2471
 *  Adafruit TB6612 1.2A DC/Stepper Motor Driver Board https://www.adafruit.com/products/2448
 *  Stepper motor NEMA-17 size 200 steps/rev, 12V 350mA https://www.adafruit.com/products/324 or equiv
 *	12 VDC 1000mA power supply  https://www.adafruit.com/products/798 or equiv
 *	5 VDC 500mA power supply https://www.adafruit.com/products/501 or equiv 
 *  FTDI or USB console cable https://www.adafruit.com/products/954 or equiv
 *  Breadboard, wire, etc.


##Software
 *  Arduino IDE with ESP8266 extension package installed
 *  Download my ESP8266 Stepper web server app from GitHub https://github.com/rgrokett/ESP8266_stepper/

See [ESP8266_Stepper.pdf](ESP8266_Stepper.pdf) for details.


