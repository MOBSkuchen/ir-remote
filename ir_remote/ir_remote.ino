#include <WiFi.h>
#include <WebServer.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#define IR_RECV_PIN 15
#define IR_SEND_PIN 4
#define SERIAL_BAUD 115200
#define MAX_SAVED 64
#define RAW_BUF_SIZE 1024

IRrecv irrecv(IR_RECV_PIN, RAW_BUF_SIZE);
IRsend irsend(IR_SEND_PIN);
WebServer server(80);
Preferences prefs;
decode_results results;

String savedNames[MAX_SAVED];
String savedData[MAX_SAVED];
int savedCount = 0;
bool serverRunning = false;

#include "index_html.h"

void loadSaved() {
  prefs.begin("signals", true);
  savedCount = prefs.getInt("count", 0);
  if (savedCount > MAX_SAVED) savedCount = MAX_SAVED;
  for (int i = 0; i < savedCount; i++) {
    savedNames[i] = prefs.getString(("n" + String(i)).c_str(), "");
    savedData[i] = prefs.getString(("d" + String(i)).c_str(), "");
  }
  prefs.end();
}

void persistSaved() {
  prefs.begin("signals", false);
  prefs.putInt("count", savedCount);
  for (int i = 0; i < savedCount; i++) {
    prefs.putString(("n" + String(i)).c_str(), savedNames[i]);
    prefs.putString(("d" + String(i)).c_str(), savedData[i]);
  }
  prefs.end();
}

String encodeRaw(decode_results *r) {
  String out = String(r->rawlen - 1);
  for (uint16_t i = 1; i < r->rawlen; i++) {
    out += "," + String(r->rawbuf[i] * kRawTick);
  }
  return out;
}

bool decodeRaw(const String &data, uint16_t *buf, uint16_t &len) {
  int idx = 0;
  int start = 0;
  for (int i = 0; i <= (int)data.length(); i++) {
    if (i == (int)data.length() || data[i] == ',') {
      String token = data.substring(start, i);
      if (idx == 0) {
        len = token.toInt();
      } else {
        if (idx - 1 < RAW_BUF_SIZE) buf[idx - 1] = token.toInt();
      }
      idx++;
      start = i + 1;
    }
  }
  return len > 0 && len < RAW_BUF_SIZE;
}

void sendCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleRoot() { server.send(200, "text/html", INDEX_HTML); }

void handleLearn() {
  sendCORS();
  irrecv.resume();
  unsigned long start = millis();
  while (!irrecv.decode(&results)) {
    if (millis() - start > 15000) {
      server.send(408, "application/json", "{\"error\":\"timeout\"}");
      return;
    }
    delay(50);
  }
  String raw = encodeRaw(&results);
  String json = "{\"protocol\":\"" + typeToString(results.decode_type) +
                "\",\"bits\":" + String(results.bits) +
                ",\"raw\":\"" + raw + "\"}";
  server.send(200, "application/json", json);
  irrecv.resume();
}

void handleSend() {
  sendCORS();
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"no body\"}");
    return;
  }
  StaticJsonDocument<4096> doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"bad json\"}");
    return;
  }
  if (doc.contains("raw")) {
    String raw = doc["raw"].as<String>();
    uint16_t buf[RAW_BUF_SIZE];
    uint16_t len = 0;
    if (!decodeRaw(raw, buf, len)) {
      server.send(400, "application/json", "{\"error\":\"bad raw data\"}");
      return;
    }
    uint16_t hz = doc["hz"] | 38;
    irsend.sendRaw(buf, len, hz);
    server.send(200, "application/json", "{\"ok\":true}");
  } else if (doc.contains("name")) {
    String name = doc["name"].as<String>();
    for (int i = 0; i < savedCount; i++) {
      if (name == savedNames[i]) {
        String raw = savedData[i];
        uint16_t buf[RAW_BUF_SIZE];
        uint16_t len = 0;
        if (!decodeRaw(raw, buf, len)) {
          server.send(500, "application/json", "{\"error\":\"bad saved raw data\"}");
          return;
        }
        uint16_t hz = doc["hz"] | 38;
        irsend.sendRaw(buf, len, hz);
        server.send(200, "application/json", "{\"ok\":true}");
      }
    }
    server.send(404, "application/json", "{\"error\":\"saved name not found\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"bad json\"}");
  }
}

void handleSave() {
  sendCORS();
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"no body\"}");
    return;
  }
  StaticJsonDocument<4096> doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"bad json\"}");
    return;
  }
  String name = doc["name"].as<String>();
  String raw = doc["raw"].as<String>();
  if (name.length() == 0 || raw.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"name and raw required\"}");
    return;
  }
  for (int i = 0; i < savedCount; i++) {
    if (savedNames[i] == name) {
      savedData[i] = raw;
      persistSaved();
      server.send(200, "application/json", "{\"ok\":true,\"updated\":true}");
      return;
    }
  }
  if (savedCount >= MAX_SAVED) {
    server.send(507, "application/json", "{\"error\":\"storage full\"}");
    return;
  }
  savedNames[savedCount] = name;
  savedData[savedCount] = raw;
  savedCount++;
  persistSaved();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleList() {
  sendCORS();
  String json = "[";
  for (int i = 0; i < savedCount; i++) {
    if (i > 0) json += ",";
    json += "{\"name\":\"" + savedNames[i] + "\",\"raw\":\"" + savedData[i] + "\"}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleDelete() {
  sendCORS();
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"no body\"}");
    return;
  }
  StaticJsonDocument<256> doc;
  deserializeJson(doc, server.arg("plain"));
  String name = doc["name"].as<String>();
  for (int i = 0; i < savedCount; i++) {
    if (savedNames[i] == name) {
      for (int j = i; j < savedCount - 1; j++) {
        savedNames[j] = savedNames[j + 1];
        savedData[j] = savedData[j + 1];
      }
      savedCount--;
      persistSaved();
      server.send(200, "application/json", "{\"ok\":true}");
      return;
    }
  }
  server.send(404, "application/json", "{\"error\":\"not found\"}");
}

void handleOptions() {
  sendCORS();
  server.send(204);
}

void startServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/learn", HTTP_GET, handleLearn);
  server.on("/send", HTTP_POST, handleSend);
  server.on("/send", HTTP_OPTIONS, handleOptions);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/save", HTTP_OPTIONS, handleOptions);
  server.on("/list", HTTP_GET, handleList);
  server.on("/delete", HTTP_POST, handleDelete);
  server.on("/delete", HTTP_OPTIONS, handleOptions);
  server.begin();
  serverRunning = true;
}

bool connectWiFi() {
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();
  if (ssid.length() == 0) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();

  if (line.startsWith("WIFI:")) {
    String payload = line.substring(5);
    int sep = payload.indexOf('|');
    if (sep < 0) {
      Serial.println("ERR:bad format");
      return;
    }
    String ssid = payload.substring(0, sep);
    String pass = payload.substring(sep + 1);

    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();

    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      delay(250);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("OK:IP:" + WiFi.localIP().toString());
      if (!serverRunning) startServer();
    } else {
      prefs.begin("wifi", false);
      prefs.remove("ssid");
      prefs.remove("pass");
      prefs.end();
      Serial.println("ERR:connection failed");
    }
  } else if (line == "PING") {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("OK:IP:" + WiFi.localIP().toString());
    } else {
      Serial.println("OK:NOCONN");
    }
  } else if (line == "RESET_WIFI") {
    prefs.begin("wifi", false);
    prefs.remove("ssid");
    prefs.remove("pass");
    prefs.end();
    Serial.println("OK:CLEARED");
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);
  Serial.println("READY");

  irsend.begin();
  irrecv.enableIRIn();
  loadSaved();

  if (connectWiFi()) {
    Serial.println("OK:IP:" + WiFi.localIP().toString());
    startServer();
  } else {
    Serial.println("NEED_WIFI");
  }
}

void loop() {
  if (serverRunning) server.handleClient();
  handleSerial();
}
