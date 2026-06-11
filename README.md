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

## Hardened server variant

The original sketch and example script are preserved for compatibility. A
separate, hardened implementation is available at:

* `sketch/StepperServerHardened/StepperServerHardened.ino`
* `sketch/StepperServerHardened/StepperConfig.example.h`
* `script/hardened/example.sh`

The hardened sketch starts with the TB6612 standby pin low, keeps the motor
disabled after a stop request, and disables it whenever WiFi disconnects.
Requests have a two-second deadline, routes and numeric values are parsed
strictly, and errors use appropriate HTTP status codes. All state-changing
commands require `POST` and an `X-API-Key` header.

Before uploading, copy `StepperConfig.example.h` to `StepperConfig.h` and
replace all three placeholders. The local configuration file is ignored by
Git to reduce the risk of committing credentials. Use an API key containing at
least 20 random characters. The API uses plain HTTP, so the key and commands
are not encrypted; operate it only on a trusted, isolated WiFi network and do
not expose port 80 to the Internet.

The hardened variant is tested with:

* Adafruit Feather HUZZAH ESP8266 board definition
  (`esp8266:esp8266:huzzah`)
* ESP8266 Arduino core 3.1.2
* Arduino Stepper library 1.1.3

Compile it with:

```sh
cp sketch/StepperServerHardened/StepperConfig.example.h \
  sketch/StepperServerHardened/StepperConfig.h
# Edit sketch/StepperServerHardened/StepperConfig.h before uploading.
arduino-cli compile \
  --fqbn esp8266:esp8266:huzzah \
  sketch/StepperServerHardened
```

Run the example client by exporting the same API key configured in the sketch
and optionally passing the ESP8266 address:

```sh
STEPPER_API_KEY='replace-with-your-key' \
  script/hardened/example.sh 192.168.1.82
```

The command endpoints are:

```text
POST /stepper/start
POST /stepper/stop
POST /stepper/rpm?value=1..60
POST /stepper/steps?value=-200..200
```

Zero steps are rejected. A movement request returns `409 Conflict` until the
motor has been explicitly started.

Wire the TB6612 and motor as described in the PDF, but verify the board revision
and supply ratings before applying power. The ESP8266 and motor-driver logic
must share a common ground. Do not feed 12 V into a HUZZAH revision whose
regulator is limited to 5 V input.


