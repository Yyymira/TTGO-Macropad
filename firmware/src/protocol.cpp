#include "protocol.h"
#include "config.h"
#include "display.h"

#include <ArduinoJson.h>

static String lineBuffer;

void protocolSetup() {
    lineBuffer.reserve(JSON_LINE_MAX);
}

void protocolSendHello() {
    JsonDocument doc;
    doc["type"] = "hello";
    doc["fw"] = "1.0.0";
    doc["buttons"] = NUM_BUTTONS;
    doc["encoders"] = NUM_ENCODERS;
    serializeJson(doc, Serial);
    Serial.println();
}

void protocolSendButton(uint8_t id, bool pressed) {
    JsonDocument doc;
    doc["type"] = "button";
    doc["id"] = id;
    doc["state"] = pressed ? "down" : "up";
    serializeJson(doc, Serial);
    Serial.println();
}

void protocolSendEncoder(uint8_t id, int8_t delta) {
    JsonDocument doc;
    doc["type"] = "encoder";
    doc["id"] = id;
    doc["delta"] = delta;
    serializeJson(doc, Serial);
    Serial.println();
}

static void handleLine(const String &line) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, line);
    if (err) return;  // línea corrupta/incompleta, se ignora

    const char *type = doc["type"];
    if (!type) return;

    if (strcmp(type, "mode") == 0) {
        const char *value = doc["value"];
        if (value && strcmp(value, "idle") == 0) {
            displaySetMode(DisplayMode::Idle);
        } else if (value && strcmp(value, "layer") == 0) {
            displaySetMode(DisplayMode::Layer);
        }
    } else if (strcmp(type, "stats") == 0) {
        displaySetStats(doc["cpu"] | -1, doc["ram"] | -1, doc["cpu_temp"] | -1,
                         doc["gpu"] | -1, doc["gpu_temp"] | -1);
    } else if (strcmp(type, "layer") == 0) {
        String labels[NUM_BUTTONS];
        JsonArray arr = doc["labels"];
        for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
            labels[i] = (i < arr.size()) ? arr[i].as<String>() : "";
        }
        String name = doc["name"] | "";
        displaySetLayer(doc["index"] | 0, name, labels);
    }
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
