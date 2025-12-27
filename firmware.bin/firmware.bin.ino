#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>

/* ================= USER SETTINGS ================= */

const char* ssid = "COFE_B";
const char* password = "COFE12345B";

// GitHub RAW firmware URL
const char* firmwareUrl =
"https://raw.githubusercontent.com/sarvindk/esp32-ota/main/firmware.bin";

/* ================================================= */

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
      Serial.println("WiFi disconnected");
      Serial.println("Retrying WiFi...");
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

  client.setInsecure();  // HTTPS without certificate (testing)

  HTTPClient http;
  http.begin(client, firmwareUrl);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("OTA failed. HTTP code: %d\n", httpCode);
    http.end();
    return;
  }

  int contentLength = http.getSize();
  bool canBegin = Update.begin(contentLength);

  if (!canBegin) {
    Serial.println("Not enough space for OTA");
    http.end();
    return;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);

  if (written == contentLength) {
    Serial.println("Written successfully");
  } else {
    Serial.println("Written partially");
  }

  if (Update.end()) {
    if (Update.isFinished()) {
      Serial.println("OTA Update Success!");
      prefs.putBool("done", true);   // mark OTA done
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

  prefs.begin("ota", false);

  WiFi.onEvent(WiFiEvent);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi...");

  // Wait max 10 seconds for WiFi
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  bool otaDone = prefs.getBool("done", false);

  if (!otaDone) {
    doOTA();   // OTA runs only once
  } else {
    Serial.println("OTA already completed. Normal run.");
  }
}

/* ================= LOOP ================= */

void loop() {
  // Your normal program code here
  delay(1000);
}
