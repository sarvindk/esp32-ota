#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>

/* ================= CONFIG ================= */
#define LED_PIN 2
#define CURRENT_VERSION "1.0.0"

/* ================= WIFI ================= */
const char* ssid     = "COFE_B";
const char* password = "COFE12345B";

/* ================= OTA URL ================= */
const char* versionUrl =
"https://raw.githubusercontent.com/sarvindk/esp32-ota/main/version.txt";

const char* firmwareUrl =
"https://raw.githubusercontent.com/sarvindk/esp32-ota/main/firmware.bin";

/* ================= GLOBAL ================= */
WiFiClientSecure client;
bool otaChecked = false;

/* ================= WIFI EVENT ================= */
void WiFiEvent(WiFiEvent_t event) {
  if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP && !otaChecked) {
    otaChecked = true;
    Serial.println("WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    checkVersionAndOTA();
  }
}

/* ================= VERSION CHECK ================= */
String getRemoteVersion() {
  client.setInsecure();
  HTTPClient http;

  Serial.println("Checking version...");
  http.begin(client, versionUrl);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Version check failed: %d\n", httpCode);
    http.end();
    return "";
  }

  String version = http.getString();
  version.trim();
  http.end();

  Serial.print("Remote version: ");
  Serial.println(version);
  return version;
}

/* ================= OTA ================= */
void doOTA() {
  Serial.println("Starting OTA...");

  HTTPClient http;
  http.begin(client, firmwareUrl);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Firmware download failed: %d\n", httpCode);
    http.end();
    return;
  }

  int contentLength = http.getSize();
  if (!Update.begin(contentLength)) {
    Serial.println("Not enough space");
    http.end();
    return;
  }

  WiFiClient* stream = http.getStreamPtr();
  uint8_t buff[128];
  size_t written = 0;

  while (written < contentLength) {
    size_t available = stream->available();
    if (available) {
      size_t read = stream->readBytes(buff, min(available, sizeof(buff)));
      Update.write(buff, read);
      written += read;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(30);
    }
  }

  if (Update.end() && Update.isFinished()) {
    Serial.println("OTA SUCCESS, rebooting...");
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("OTA FAILED");
  }

  http.end();
}

/* ================= LOGIC ================= */
void checkVersionAndOTA() {
  String remoteVersion = getRemoteVersion();
  if (remoteVersion == "") return;

  if (remoteVersion != CURRENT_VERSION) {
    Serial.println("New version found!");
    doOTA();
  } else {
    Serial.println("Already latest version");
    digitalWrite(LED_PIN, HIGH);
  }
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.onEvent(WiFiEvent);
  WiFi.begin(ssid, password);

  Serial.println("Connecting WiFi...");
}

/* ================= LOOP ================= */
void loop() {
  delay(1000);
}