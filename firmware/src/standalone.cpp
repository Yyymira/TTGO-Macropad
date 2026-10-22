#include "standalone.h"

#include "i18n.h"

#include "ble_kbd.h"
#include "config.h"
#include "display.h"
#include "settings.h"

#include <ArduinoJson.h>

// Formato compacto que manda la PC (ver controller.py):
// [{"n": "General", "b": [{"a": "key", "k": ["ctrl", "t"]}, {"a": "layer_set", "i": 1}, ...],
//   "e": [{"cw": [...], "ccw": [...]}, ...]}, ...]
static JsonDocument keymap;
static String keymapJson;
static uint8_t layerIdx = 0;
static uint32_t lastActivityMs = 0;
static bool active = true;

static size_t layerCount() { return keymap.is<JsonArray>() ? keymap.as<JsonArrayConst>().size() : 0; }

static String labelFor(JsonVariantConst action) {
    const char *kind = action["a"] | "none";
    const char *alias = action["l"] | "";
    if (alias[0] && strcmp(kind, "none") != 0) return alias;
    if (strcmp(kind, "key") == 0) {
        String s;
        for (JsonVariantConst k : action["k"].as<JsonArrayConst>()) {
            if (s.length()) s += "+";
            String part = k | "";
            part.toUpperCase();
            s += part;
        }
        return s.substring(0, 24);
    }
    if (strcmp(kind, "layer_next") == 0) return tr(Str::LayerNext);
    if (strcmp(kind, "layer_prev") == 0) return tr(Str::LayerPrev);
    if (strcmp(kind, "layer_set") == 0) return String(tr(Str::Layer)) + " " + String(action["i"] | 0);
    return "";
}

static void showLayer() {
    JsonVariantConst layer = keymap[layerIdx];
    JsonArrayConst buttons = layer["b"];
    String labels[NUM_BUTTONS];
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) labels[i] = i < buttons.size() ? labelFor(buttons[i]) : "";
    displaySetLayer(layerIdx, String(layer["n"] | ""), labels);
    displaySetMode(DisplayMode::Layer);
}

void standaloneSetKeymap(const String &layersJson) {
    if (layersJson == keymapJson) return;
    keymapJson = layersJson;
    keymap.clear();
    deserializeJson(keymap, keymapJson);
    keymapSave(keymapJson);
    if (layerIdx >= layerCount()) layerIdx = 0;
}

void standaloneBegin() {
    keymapJson = keymapLoad();
    if (keymapJson.length()) deserializeJson(keymap, keymapJson);
}

void standaloneActivate(bool on) {
    active = on;
    if (on) displaySetMode(DisplayMode::Idle);
}

void standaloneButton(uint8_t id) {
    size_t n = layerCount();
    if (!active || n == 0) return;
    lastActivityMs = millis();
    uint8_t before = layerIdx;

    JsonVariantConst action = keymap[layerIdx]["b"][id];
    const char *kind = action["a"] | "none";
    if (strcmp(kind, "key") == 0) {
        bleSendKeys(action["k"]);
    } else if (strcmp(kind, "layer_next") == 0) {
        layerIdx = (layerIdx + 1) % n;
    } else if (strcmp(kind, "layer_prev") == 0) {
        layerIdx = (layerIdx + n - 1) % n;
    } else if (strcmp(kind, "layer_set") == 0) {
        layerIdx = constrain((int)(action["i"] | 0), 0, (int)n - 1);
    }

    if (layerIdx != before || displayGetMode() != DisplayMode::Layer) showLayer();
}

void standaloneEncoder(uint8_t id, int8_t delta) {
    if (!active || layerCount() == 0) return;
    lastActivityMs = millis();
    JsonVariantConst enc = keymap[layerIdx]["e"][id];
    bleSendKeys(delta > 0 ? enc["cw"] : enc["ccw"]);
    if (displayGetMode() != DisplayMode::Layer) showLayer();
}

void standaloneLoop() {
    if (!active || displayGetMode() != DisplayMode::Layer) return;
    if (millis() - lastActivityMs >= (uint32_t)settings.idleTimeoutSec * 1000) displaySetMode(DisplayMode::Idle);
}
