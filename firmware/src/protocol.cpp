#include "protocol.h"

#include "ble_kbd.h"
#include "config.h"
#include "display.h"
#include "net.h"
#include "settings.h"
#include "standalone.h"

#include <ArduinoJson.h>

static String lineBuffer;
static uint32_t lastHostMs = 0;
static bool hostSeen = false;

void protocolSetup() { lineBuffer.reserve(JSON_LINE_MAX); }

bool protocolHostAlive(uint32_t timeoutMs) { return hostSeen && millis() - lastHostMs < timeoutMs; }

static void sendDoc(JsonDocument &doc) {
    serializeJson(doc, Serial);
    Serial.println();
}

void protocolSendHello() {
    JsonDocument doc;
    doc["type"] = "hello";
    doc["fw"] = "1.1.0";
    doc["buttons"] = NUM_BUTTONS;
    doc["encoders"] = NUM_ENCODERS;
    sendDoc(doc);
}

void protocolSendButton(uint8_t id, bool pressed) {
    JsonDocument doc;
    doc["type"] = "button";
    doc["id"] = id;
    doc["state"] = pressed ? "down" : "up";
    sendDoc(doc);
}

void protocolSendEncoder(uint8_t id, int8_t delta) {
    JsonDocument doc;
    doc["type"] = "encoder";
    doc["id"] = id;
    doc["delta"] = delta;
    sendDoc(doc);
}

void protocolSendNet() {
    WeatherData w;
    JsonDocument doc;
    doc["type"] = "net";
    doc["wifi"] = netWifiConnected();
    doc["ip"] = netIp();
    doc["ble"] = bleConnected();
    doc["weather"] = netGetWeather(w);
    sendDoc(doc);
}

static bool sameFloat(float a, float b) { return (isnan(a) && isnan(b)) || fabsf(a - b) < 1e-5f; }

static void applySettings(JsonDocument &doc) {
    bool changed = false, wifiChanged = false, locationChanged = false;

    auto setString = [&](const char *key, String &field, bool *flag) {
        if (!doc[key].is<const char *>()) return;
        String value = doc[key].as<const char *>();
        if (value == field) return;
        field = value;
        changed = true;
        if (flag) *flag = true;
    };
    setString("wifi_ssid", settings.wifiSsid, &wifiChanged);
    setString("wifi_pass", settings.wifiPass, &wifiChanged);
    setString("ota_pass", settings.otaPass, nullptr);

    float lat = doc["lat"] | NAN, lon = doc["lon"] | NAN;
    if (!sameFloat(lat, settings.lat) || !sameFloat(lon, settings.lon)) {
        settings.lat = lat;
        settings.lon = lon;
        changed = locationChanged = true;
    }

    const char *view = doc["idle_view"] | "both";
    IdleView newView = strcmp(view, "stats") == 0     ? IdleView::Stats
                       : strcmp(view, "weather") == 0 ? IdleView::Weather
                                                      : IdleView::Both;
    if (newView != settings.idleView) {
        settings.idleView = newView;
        changed = true;
    }

    uint16_t timeout = doc["idle_timeout"] | 15;
    int32_t offset = doc["utc_offset"] | INT32_MIN;
    if (timeout != settings.idleTimeoutSec || offset != settings.utcOffset) {
        settings.idleTimeoutSec = timeout;
        settings.utcOffset = offset;
        changed = true;
    }

    if (changed) settingsSave();  // solo si cambió algo: cuida la flash
    netApplySettings(wifiChanged, locationChanged);
    displayRefresh();
}

static void handleLine(const String &line) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, line);
    if (err) return;  // línea corrupta/incompleta, se ignora

    const char *type = doc["type"];
    if (!type) return;
    lastHostMs = millis();
    hostSeen = true;

    if (strcmp(type, "mode") == 0) {
        const char *value = doc["value"];
        if (value && strcmp(value, "idle") == 0) {
            displaySetMode(DisplayMode::Idle);
        } else if (value && strcmp(value, "layer") == 0) {
            displaySetMode(DisplayMode::Layer);
        }
    } else if (strcmp(type, "stats") == 0) {
        displaySetStats(doc["cpu"] | -1, doc["ram"] | -1, doc["cpu_temp"] | -1,
                         doc["gpu"] | -1, doc["gpu_temp"] | -1, doc["vram"] | -1);
    } else if (strcmp(type, "layer") == 0) {
        String labels[NUM_BUTTONS];
        JsonArray arr = doc["labels"];
        for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
            labels[i] = (i < arr.size()) ? arr[i].as<String>() : "";
        }
        String name = doc["name"] | "";
        displaySetLayer(doc["index"] | 0, name, labels);
    } else if (strcmp(type, "settings") == 0) {
        applySettings(doc);
    } else if (strcmp(type, "keymap") == 0) {
        String layers;
        serializeJson(doc["layers"], layers);
        standaloneSetKeymap(layers);
    }
    // "ping": solo sirve para que la placa sepa que la PC sigue ahí.
}

void protocolLoop() {
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\n') {
            if (lineBuffer.length() > 0) {
                handleLine(lineBuffer);
                lineBuffer = "";
            }
        } else if (c != '\r') {
            if (lineBuffer.length() < JSON_LINE_MAX) lineBuffer += c;
        }
    }
}
