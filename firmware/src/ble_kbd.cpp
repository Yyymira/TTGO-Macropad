#include "ble_kbd.h"

#include <BleKeyboard.h>

static BleKeyboard kbd("TTGO Macropad", "Yyymira", 100);

struct NamedKey {
    const char *name;
    uint8_t key;
};

static const NamedKey KEYS[] = {
    {"ctrl", KEY_LEFT_CTRL},    {"control", KEY_LEFT_CTRL}, {"rctrl", KEY_RIGHT_CTRL},
    {"shift", KEY_LEFT_SHIFT},  {"rshift", KEY_RIGHT_SHIFT}, {"alt", KEY_LEFT_ALT},
    {"ralt", KEY_RIGHT_ALT},    {"super", KEY_LEFT_GUI},    {"meta", KEY_LEFT_GUI},
    {"win", KEY_LEFT_GUI},      {"tab", KEY_TAB},           {"enter", KEY_RETURN},
    {"return", KEY_RETURN},     {"esc", KEY_ESC},           {"escape", KEY_ESC},
    {"space", ' '},             {"backspace", KEY_BACKSPACE}, {"delete", KEY_DELETE},
    {"del", KEY_DELETE},        {"up", KEY_UP_ARROW},       {"down", KEY_DOWN_ARROW},
    {"left", KEY_LEFT_ARROW},   {"right", KEY_RIGHT_ARROW}, {"home", KEY_HOME},
    {"end", KEY_END},           {"pageup", KEY_PAGE_UP},    {"pagedown", KEY_PAGE_DOWN},
    {"f1", KEY_F1},   {"f2", KEY_F2},   {"f3", KEY_F3},   {"f4", KEY_F4},
    {"f5", KEY_F5},   {"f6", KEY_F6},   {"f7", KEY_F7},   {"f8", KEY_F8},
    {"f9", KEY_F9},   {"f10", KEY_F10}, {"f11", KEY_F11}, {"f12", KEY_F12},
};

struct NamedMedia {
    const char *name;
    const uint8_t *key;
};

// Brillo no existe en el reporte multimedia de BLE HID: esas teclas se ignoran.
static const NamedMedia MEDIA[] = {
    {"play", KEY_MEDIA_PLAY_PAUSE},  {"playpause", KEY_MEDIA_PLAY_PAUSE}, {"pause", KEY_MEDIA_PLAY_PAUSE},
    {"stop", KEY_MEDIA_STOP},        {"next", KEY_MEDIA_NEXT_TRACK},      {"nextsong", KEY_MEDIA_NEXT_TRACK},
    {"prev", KEY_MEDIA_PREVIOUS_TRACK}, {"previoussong", KEY_MEDIA_PREVIOUS_TRACK},
    {"mute", KEY_MEDIA_MUTE},        {"vol+", KEY_MEDIA_VOLUME_UP},       {"volup", KEY_MEDIA_VOLUME_UP},
    {"volumeup", KEY_MEDIA_VOLUME_UP}, {"vol-", KEY_MEDIA_VOLUME_DOWN},   {"voldown", KEY_MEDIA_VOLUME_DOWN},
    {"volumedown", KEY_MEDIA_VOLUME_DOWN},
};

struct Resolved {
    const uint8_t *media = nullptr;
    uint8_t key = 0;
};

static bool resolve(String name, Resolved &out) {
    name.trim();
    name.toLowerCase();
    for (const auto &k : KEYS) {
        if (name == k.name) {
            out.key = k.key;
            return true;
        }
    }
    for (const auto &m : MEDIA) {
        if (name == m.name) {
            out.media = m.key;
            return true;
        }
    }
    if (name.length() == 1 && name[0] >= 0x20 && name[0] < 0x7F) {
        out.key = (uint8_t)name[0];
        return true;
    }
    return false;
}

void bleSetup() { kbd.begin(); }

bool bleConnected() { return kbd.isConnected(); }

void bleSendKeys(JsonArrayConst keys) {
    if (!kbd.isConnected() || keys.isNull() || keys.size() == 0) return;
    Resolved combo[8];
    size_t n = 0;
    for (JsonVariantConst v : keys) {
        if (n >= 8) break;
        if (!resolve(String(v | ""), combo[n])) return;  // tecla desconocida: no se manda nada
        n++;
    }
    for (size_t i = 0; i < n; i++) {
        if (combo[i].media) {
            kbd.press(combo[i].media);
        } else {
            kbd.press(combo[i].key);
        }
    }
    delay(15);
    kbd.releaseAll();
}
