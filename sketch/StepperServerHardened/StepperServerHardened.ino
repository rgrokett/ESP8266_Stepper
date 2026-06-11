/*
 * Hardened ESP8266 stepper server.
 *
 * State-changing routes require POST and an X-API-Key header. This is still
 * plain HTTP, so use it only on a trusted, isolated network.
 */

#include <ESP8266WiFi.h>
#include <Stepper.h>

#include "StepperConfig.h"

constexpr int STEPS_PER_REVOLUTION = 200;
constexpr int MAX_RPM = 60;
constexpr int MAX_STEPS_PER_REQUEST = 200;
constexpr int STEP_YIELD_DELAY_MS = 1;

constexpr uint8_t STBY_PIN = 5;
constexpr uint8_t LED_PIN = 0;
constexpr uint8_t MOTOR_PIN_1 = 16;
constexpr uint8_t MOTOR_PIN_2 = 14;
constexpr uint8_t MOTOR_PIN_3 = 12;
constexpr uint8_t MOTOR_PIN_4 = 13;

constexpr uint16_t HTTP_PORT = 80;
constexpr unsigned long REQUEST_TIMEOUT_MS = 2000;
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 10000;
constexpr size_t MAX_REQUEST_LINE_LENGTH = 160;
constexpr size_t MAX_HEADER_LINE_LENGTH = 256;
constexpr uint8_t MAX_HEADER_COUNT = 20;

Stepper stepper(
    STEPS_PER_REVOLUTION,
    MOTOR_PIN_1,
    MOTOR_PIN_2,
    MOTOR_PIN_3,
    MOTOR_PIN_4);
WiFiServer server(HTTP_PORT);

bool motorEnabled = false;
bool serverStarted = false;
unsigned long lastWifiAttemptMs = 0;

void setMotorEnabled(bool enabled) {
  motorEnabled = enabled;
  digitalWrite(STBY_PIN, enabled ? HIGH : LOW);
}

void blink() {
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
}

bool configurationIsValid() {
  return strcmp(WIFI_SSID, "CHANGE_ME") != 0 &&
         strcmp(WIFI_PASSWORD, "CHANGE_ME") != 0 &&
         strcmp(STEPPER_API_KEY, "CHANGE_ME_TO_A_LONG_RANDOM_VALUE") != 0 &&
         strlen(STEPPER_API_KEY) >= 20;
}

bool constantTimeEquals(const String& supplied, const char* expected) {
  const size_t suppliedLength = supplied.length();
  const size_t expectedLength = strlen(expected);
  const size_t comparisonLength =
      suppliedLength > expectedLength ? suppliedLength : expectedLength;
  size_t difference = suppliedLength ^ expectedLength;

  for (size_t i = 0; i < comparisonLength; ++i) {
    const char suppliedChar = i < suppliedLength ? supplied[i] : 0;
    const char expectedChar = i < expectedLength ? expected[i] : 0;
    difference |= static_cast<unsigned char>(
        suppliedChar ^ expectedChar);
  }

  return difference == 0;
}

bool readHttpLine(
    WiFiClient& client,
    String& line,
    size_t maximumLength,
    unsigned long deadlineMs) {
  line = "";

  while (static_cast<long>(deadlineMs - millis()) > 0) {
    while (client.available()) {
      const char character = static_cast<char>(client.read());
      if (character == '\n') {
        if (line.endsWith("\r")) {
          line.remove(line.length() - 1);
        }
        return true;
      }

      if (line.length() >= maximumLength) {
        return false;
      }
      line += character;
    }

    if (!client.connected()) {
      return false;
    }
    delay(1);
  }

  return false;
}

void sendResponse(
    WiFiClient& client,
    int statusCode,
    const __FlashStringHelper* reason,
    const String& body,
    const __FlashStringHelper* extraHeader = nullptr) {
  client.print(F("HTTP/1.1 "));
  client.print(statusCode);
  client.print(' ');
  client.println(reason);
  client.println(F("Content-Type: text/plain; charset=utf-8"));
  client.println(F("Cache-Control: no-store"));
  client.println(F("Connection: close"));
  if (extraHeader != nullptr) {
    client.println(extraHeader);
  }
  client.print(F("Content-Length: "));
  client.println(body.length());
  client.println();
  client.print(body);
}

bool parseRequestLine(
    const String& requestLine,
    String& method,
    String& target) {
  const int firstSpace = requestLine.indexOf(' ');
  const int secondSpace = requestLine.indexOf(' ', firstSpace + 1);
  if (firstSpace <= 0 || secondSpace <= firstSpace + 1) {
    return false;
  }

  method = requestLine.substring(0, firstSpace);
  target = requestLine.substring(firstSpace + 1, secondSpace);
  const String version = requestLine.substring(secondSpace + 1);
  return version == F("HTTP/1.0") || version == F("HTTP/1.1");
}

bool parseIntegerParameter(
    const String& target,
    const char* route,
    int minimum,
    int maximum,
    int& value) {
  const String prefix = String(route) + F("?value=");
  if (!target.startsWith(prefix)) {
    return false;
  }

  const String text = target.substring(prefix.length());
  if (text.length() == 0) {
    return false;
  }

  char* end = nullptr;
  const long parsed = strtol(text.c_str(), &end, 10);
  if (*end != '\0' || parsed < minimum || parsed > maximum) {
    return false;
  }

  value = static_cast<int>(parsed);
  return true;
}

String usage() {
  String body;
  body.reserve(420);
  body += F("Hardened stepper API\n\n");
  body += F("All commands require POST and X-API-Key.\n");
  body += F("POST /stepper/start\n");
  body += F("POST /stepper/stop\n");
  body += F("POST /stepper/rpm?value=1..");
  body += MAX_RPM;
  body += '\n';
  body += F("POST /stepper/steps?value=-");
  body += MAX_STEPS_PER_REQUEST;
  body += F("..");
  body += MAX_STEPS_PER_REQUEST;
  body += F(" (zero is rejected)\n");
  return body;
}

void handleClient(WiFiClient& client) {
  const unsigned long deadlineMs = millis() + REQUEST_TIMEOUT_MS;
  String requestLine;
  if (!readHttpLine(
          client, requestLine, MAX_REQUEST_LINE_LENGTH, deadlineMs)) {
    sendResponse(client, 408, F("Request Timeout"), F("Request timed out\n"));
    return;
  }

  String method;
  String target;
  if (!parseRequestLine(requestLine, method, target)) {
    sendResponse(client, 400, F("Bad Request"), F("Malformed request line\n"));
    return;
  }

  String suppliedApiKey;
  bool headersComplete = false;
  for (uint8_t count = 0; count < MAX_HEADER_COUNT; ++count) {
    String header;
    if (!readHttpLine(client, header, MAX_HEADER_LINE_LENGTH, deadlineMs)) {
      sendResponse(client, 408, F("Request Timeout"), F("Request timed out\n"));
      return;
    }
    if (header.length() == 0) {
      headersComplete = true;
      break;
    }

    const int separator = header.indexOf(':');
    if (separator <= 0) {
      sendResponse(client, 400, F("Bad Request"), F("Malformed header\n"));
      return;
    }

    String name = header.substring(0, separator);
    String value = header.substring(separator + 1);
    value.trim();
    if (name.equalsIgnoreCase(F("X-API-Key"))) {
      suppliedApiKey = value;
    }
  }

  if (!headersComplete) {
    sendResponse(
        client, 431, F("Request Header Fields Too Large"),
        F("Too many headers\n"));
    return;
  }

  if (method == F("GET") && target == F("/")) {
    sendResponse(client, 200, F("OK"), usage());
    return;
  }

  if (method != F("POST")) {
    sendResponse(
        client, 405, F("Method Not Allowed"),
        F("Motor commands require POST\n"), F("Allow: GET, POST"));
    return;
  }

  if (!constantTimeEquals(suppliedApiKey, STEPPER_API_KEY)) {
    sendResponse(client, 401, F("Unauthorized"), F("Invalid API key\n"));
    return;
  }

  if (target == F("/stepper/stop")) {
    setMotorEnabled(false);
    sendResponse(client, 200, F("OK"), F("OK: MOTORS OFF\n"));
    return;
  }

  if (target == F("/stepper/start")) {
    setMotorEnabled(true);
    blink();
    sendResponse(client, 200, F("OK"), F("OK: MOTORS ON\n"));
    return;
  }

  int value = 0;
  if (target.startsWith(F("/stepper/rpm"))) {
    if (!parseIntegerParameter(target, "/stepper/rpm", 1, MAX_RPM, value)) {
      sendResponse(client, 400, F("Bad Request"), F("Invalid RPM value\n"));
      return;
    }
    stepper.setSpeed(value);
    sendResponse(client, 200, F("OK"), "OK: RPM = " + String(value) + "\n");
    return;
  }

  if (target.startsWith(F("/stepper/steps"))) {
    if (!parseIntegerParameter(
            target, "/stepper/steps", -MAX_STEPS_PER_REQUEST,
            MAX_STEPS_PER_REQUEST, value) ||
        value == 0) {
      sendResponse(client, 400, F("Bad Request"), F("Invalid step value\n"));
      return;
    }
    if (!motorEnabled) {
      sendResponse(
          client, 409, F("Conflict"),
          F("Motor is disabled; call /stepper/start first\n"));
      return;
    }

    const int direction = value > 0 ? 1 : -1;
    const int count = abs(value);
    for (int step = 0; step < count; ++step) {
      stepper.step(direction);
      delay(STEP_YIELD_DELAY_MS);
    }
    sendResponse(
        client, 200, F("OK"), "OK: STEPS = " + String(value) + "\n");
    return;
  }

  sendResponse(client, 404, F("Not Found"), F("Unknown endpoint\n"));
}

void maintainWifi() {
  static wl_status_t previousStatus = WL_IDLE_STATUS;
  const wl_status_t currentStatus = WiFi.status();

  if (currentStatus != WL_CONNECTED) {
    if (previousStatus == WL_CONNECTED) {
      Serial.println(F("WiFi disconnected; disabling motor"));
      setMotorEnabled(false);
    }

    const unsigned long now = millis();
    if (now - lastWifiAttemptMs >= WIFI_RETRY_INTERVAL_MS) {
      lastWifiAttemptMs = now;
      Serial.println(F("Attempting WiFi connection"));
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  } else if (previousStatus != WL_CONNECTED) {
    Serial.print(F("WiFi connected: "));
    Serial.println(WiFi.localIP());
    if (!serverStarted) {
      server.begin();
      server.setNoDelay(true);
      serverStarted = true;
      Serial.println(F("HTTP server started"));
    }
  }

  previousStatus = currentStatus;
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  pinMode(STBY_PIN, OUTPUT);
  setMotorEnabled(false);
  stepper.setSpeed(MAX_RPM);

  if (!configurationIsValid()) {
    Serial.println(F("Invalid StepperConfig.h; motor will remain disabled"));
    return;
  }

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  lastWifiAttemptMs = millis() - WIFI_RETRY_INTERVAL_MS;
  maintainWifi();
}

void loop() {
  if (!configurationIsValid()) {
    delay(1000);
    return;
  }

  maintainWifi();
  if (WiFi.status() != WL_CONNECTED || !serverStarted) {
    delay(10);
    return;
  }

  WiFiClient client = server.available();
  if (!client) {
    delay(1);
    return;
  }

  handleClient(client);
  client.stop();
}
