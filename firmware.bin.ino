#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>

/* ================= PIN CONFIG ================= */
#define LED_PIN 2   // Onboard LED

/* ================= WIFI CONFIG ================= */
const char* ssid     = "COFE_B";
const char* password = "COFE12345B";

/* ================= OTA CONFIG ================= */
// GitHub RAW firmware URL
const char* firmwareUrl =
"https://raw.githubusercontent.com/sarvindk/esp32-ota/main/firmware.bin";

/* ================= GLOBAL OBJECTS ================= */
WiFiClientSecure client;
Preferences prefs;

/* ================= WIFI EVENT HANDLER ================= */
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi connected");
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi disconnected. Reconnecting...");
      WiFi.reconnect();
      break;

    default:
      break;
  }
}

/* ================= OTA FUNCTION ================= */
void doOTA() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. OTA skipped.");
    return;
  }

  Serial.println("Starting OTA update...");
  client.setInsecure();   // HTTPS (testing)

  HTTPClient http;
  http.begin(client, firmwareUrl);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("OTA failed. HTTP code: %d\n", httpCode);
    http.end();
    return;
  }

  int contentLength = http.getSize();
  Serial.printf("Firmware Size: %d bytes\n", contentLength);

  if (!Update.begin(contentLength)) {
    Serial.println("Not enough space for OTA");
    http.end();
    return;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t written = 0;
  uint8_t buff[128];

  // LED BLINK while downloading
  while (written < contentLength) {
    size_t available = stream->available();
    if (available) {
      size_t readBytes = stream->readBytes(buff, min(available, sizeof(buff)));
      Update.write(buff, readBytes);
      written += readBytes;

      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // blink
      delay(50);
    }
  }

  if (Update.end()) {
    if (Update.isFinished()) {
      Serial.println("OTA Update Success!");
      prefs.putBool("done", true);   // Mark OTA done
      delay(1000);
      ESP.restart();
    } else {
      Serial.println("OTA not finished");
    }
  } else {
    Serial.printf("OTA Error: %d\n", Update.getError());
  }

  http.end();
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);   // LED OFF (Before OTA)

  prefs.begin("ota", false);

  WiFi.onEvent(WiFiEvent);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi...");

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  bool otaDone = prefs.getBool("done", false);

  if (!otaDone) {
    doOTA();   // OTA runs only once
  } else {
    Serial.println("OTA already completed.");
    digitalWrite(LED_PIN, HIGH);  // LED ON after OTA
  }
}

/* ================= LOOP ================= */
void loop() {
  // Normal application code
  delay(1000);
}
