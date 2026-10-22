#include "settings.h"

#include <Preferences.h>

#include <memory>

DeviceSettings settings;

static Preferences prefs;
static const char *NS = "macropad";

void settingsLoad() {
    prefs.begin(NS, true);
    settings.wifiSsid = prefs.getString("ssid", "");
    settings.wifiPass = prefs.getString("pass", "");
    settings.otaPass = prefs.getString("ota", "");
    settings.lat = prefs.getFloat("lat", NAN);
    settings.lon = prefs.getFloat("lon", NAN);
    settings.utcOffset = prefs.getInt("utc", INT32_MIN);
    settings.idleView = static_cast<IdleView>(prefs.getUChar("view", static_cast<uint8_t>(IdleView::Both)));
    settings.idleTimeoutSec = prefs.getUShort("idle", 15);
    settings.statsSec = prefs.getUShort("tstat", 10);
    settings.weatherSec = prefs.getUShort("twx", 10);
    settings.mediaSec = prefs.getUShort("tmed", 10);
    settings.lang = static_cast<Lang>(prefs.getUChar("lang", static_cast<uint8_t>(Lang::En)));
    prefs.end();
}

void settingsSave() {
    prefs.begin(NS, false);
    prefs.putString("ssid", settings.wifiSsid);
    prefs.putString("pass", settings.wifiPass);
    prefs.putString("ota", settings.otaPass);
    prefs.putFloat("lat", settings.lat);
    prefs.putFloat("lon", settings.lon);
    prefs.putInt("utc", settings.utcOffset);
    prefs.putUChar("view", static_cast<uint8_t>(settings.idleView));
    prefs.putUShort("idle", settings.idleTimeoutSec);
    prefs.putUShort("tstat", settings.statsSec);
    prefs.putUShort("twx", settings.weatherSec);
    prefs.putUShort("tmed", settings.mediaSec);
    prefs.putUChar("lang", static_cast<uint8_t>(settings.lang));
    prefs.end();
}

String keymapLoad() {
    prefs.begin(NS, true);
    size_t len = prefs.getBytesLength("keymap");
    String out;
    if (len > 0) {
        std::unique_ptr<char[]> buf(new char[len + 1]);
        prefs.getBytes("keymap", buf.get(), len);
        buf[len] = '\0';
        out = buf.get();
    }
    prefs.end();
    return out;
}

void keymapSave(const String &json) {
    prefs.begin(NS, false);
    prefs.putBytes("keymap", json.c_str(), json.length());
    prefs.end();
}
