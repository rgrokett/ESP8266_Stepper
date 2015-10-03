/*
 *  Controller for Stepper Motor
 *  This sketch sets up a web server on port 80
 *  The server will control the Adafruit TB6612 Driver board depending on the request
 *  See http://server_ip/ for usage (or see docs for more info)
 *  The server_ip is the IP address of the ESP8266 module which will be 
 *  printed to Serial and the LED blinked when the module is connected.
 *
 *  Version 1.1 2015.10.01 R. Grokett
 *  - Added support for STBY pin 
 */

#include <ESP8266WiFi.h>
#include <Stepper.h>

// -- USER EDIT -- 
const char* ssid     = "MY_SSID";    // YOUR WIFI SSID
const char* password = "MY_PASS";    // YOUR WIFI PASSWORD 

// change this to your motor if not NEMA-17 200 step
#define STEPS 200  // Max steps for one revolution
#define RPM 60     // Max RPM
#define DELAY 1    // Delay to allow Wifi to work
// -- END --


int STBY = 5;     // GPIO 5 TB6612 Standby
int LED = 0;      // GPIO 0 (built-in LED)

// GPIO Pins for Motor Driver board
Stepper stepper(STEPS, 16, 14, 12, 13);

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(10);

  // prepare onboard LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  // prepare STBY GPIO and turn on Motors
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);
  
  // Set default speed to Max (doesn't move motor)
  stepper.setSpeed(RPM);
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("STEPPER: Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  // Blink onboard LED to signify its connected
  blink();
  blink();
  blink();
  blink();
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  String respMsg = "";    // HTTP Response Message
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // Match the request 
  if (req.indexOf("/stepper/stop") != -1) {
    digitalWrite(STBY, LOW);
    respMsg = "OK: MOTORS OFF";
  } 
  else if (req.indexOf("/stepper/start") != -1) {
    digitalWrite(STBY, HIGH);
    blink();
    respMsg = "OK: MOTORS ON";
  } 
  else if (req.indexOf("/stepper/rpm") != -1) {
    int rpm = getValue(req);
    if ((rpm < 1) || (rpm > RPM)) {
      respMsg = "ERROR: rpm out of range 1 to "+ String(RPM);
    } else {
      stepper.setSpeed(rpm);
      respMsg = "OK: RPM = "+String(rpm);
    }
  }
  // This is just a simplistic method of handling + or - number steps...
  else if (req.indexOf("/stepper/steps") != -1) {
    int steps = getValue(req);
    if ((steps == 0) || (steps < 0 - STEPS) || ( steps > STEPS )) {
      respMsg = "ERROR: steps out of range ";
    } else {  
      digitalWrite(STBY, HIGH);       // Make sure motor is on
      respMsg = "OK: STEPS = "+String(steps);
      delay(DELAY); 
      if ( steps > 0) { // Forward
        for (int i=0;i<steps;i++) {   // This loop is needed to allow Wifi to not be blocked by step
          stepper.step(1);
          delay(DELAY);   
        }
      } else {         // Reverse
          for (int i=0;i>steps;i--) {   // This loop is needed to allow Wifi to not be blocked by step
            stepper.step(-1);
            delay(DELAY); 
          }  
      }
    }
  }
  else {
    respMsg = printUsage();
  }
    
  client.flush();

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
  if (respMsg.length() > 0)
    s += respMsg;
  else
    s += "OK";
   
  s += "\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disconnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

int getValue(String req) {
  int val_start = req.indexOf('?');
  int val_end   = req.indexOf(' ', val_start + 1);
  if (val_start == -1 || val_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return(0);
  }
  req = req.substring(val_start + 1, val_end);
  Serial.print("Request: ");
  Serial.println(req);
   
  return(req.toInt());
}

String printUsage() {
  // Prepare the usage response
  String s = "Stepper usage:\n";
  s += "http://{ip_address}/stepper/stop\n";
  s += "http://{ip_address}/stepper/start\n";
  s += "http://{ip_address}/stepper/rpm?[1 to " + String(RPM) + "]\n";
  s += "http://{ip_address}/stepper/steps?[-" + String(STEPS) + " to " + String(STEPS) +"]\n";
  return(s);
}
void blink() {
  digitalWrite(LED, LOW);
  delay(100); 
  digitalWrite(LED, HIGH);
  delay(100);
}

