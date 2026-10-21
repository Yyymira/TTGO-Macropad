#include "net.h"

#include "config.h"
#include "display.h"
#include "settings.h"

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <WiFi.h>

static SemaphoreHandle_t weatherMutex;
static WeatherData weather;
static volatile bool weatherUpdated = false;
static volatile bool refreshRequested = true;
static volatile bool otaRunning = false;
static volatile int32_t configuredOffset = INT32_MIN;
static bool otaStarted = false;

static void applyTimezone(int32_t offset) {
    if (offset == INT32_MIN || offset == configuredOffset) return;
    configuredOffset = offset;
    configTime(offset, 0, "pool.ntp.org", "time.google.com");
}

// Día de la semana (0 = domingo) para una fecha "AAAA-MM-DD" (Sakamoto).
static uint8_t weekdayOf(const char *ymd) {
    int y = 0, m = 1, d = 1;
    sscanf(ymd, "%d-%d-%d", &y, &m, &d);
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

static bool fetchWeather() {
    // Open-Meteo: gratis y sin API key. Por HTTP simple: son datos públicos.
    String url = String("http://api.open-meteo.com/v1/forecast?latitude=") + String(settings.lat, 4) +
                 "&longitude=" + String(settings.lon, 4) +
                 "&current=temperature_2m,relative_humidity_2m,weather_code"
                 "&daily=weather_code,temperature_2m_max,temperature_2m_min"
                 "&timezone=auto&forecast_days=3";
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(8000);
    if (!http.begin(client, url)) return false;
    int status = http.GET();
    if (status != 200) {
        http.end();
        return false;
    }
    String body = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, body)) return false;

    WeatherData w;
    w.valid = true;
    w.temp = doc["current"]["temperature_2m"] | 0.0f;
    w.humidity = doc["current"]["relative_humidity_2m"] | 0;
    w.code = doc["current"]["weather_code"] | 0;
    JsonArrayConst dates = doc["daily"]["time"];
    for (size_t i = 0; i < 3 && i < dates.size(); i++) {
        w.days[i].wday = weekdayOf(dates[i] | "2000-01-01");
        w.days[i].code = doc["daily"]["weather_code"][i] | 0;
        w.days[i].tmax = doc["daily"]["temperature_2m_max"][i] | 0.0f;
        w.days[i].tmin = doc["daily"]["temperature_2m_min"][i] | 0.0f;
    }

    xSemaphoreTake(weatherMutex, portMAX_DELAY);
    weather = w;
    xSemaphoreGive(weatherMutex);

    // La zona horaria de la ubicación del clima manda sobre la de la PC.
    applyTimezone(doc["utc_offset_seconds"] | INT32_MIN);
    weatherUpdated = true;
    return true;
}

static void weatherTask(void *) {
    uint32_t lastOk = 0;
    for (;;) {
        bool due = refreshRequested || lastOk == 0 || millis() - lastOk >= WEATHER_PERIOD_MS;
        if (due && !otaRunning && WiFi.isConnected() && !isnan(settings.lat) && !isnan(settings.lon)) {
            refreshRequested = false;
            if (fetchWeather()) {
                lastOk = millis();
            } else {
                vTaskDelay(pdMS_TO_TICKS(WEATHER_RETRY_MS));
                continue;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void startWifi() {
    WiFi.disconnect();
    if (settings.wifiSsid.length() > 0) WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPass.c_str());
}

static void startOta() {
    ArduinoOTA.setHostname(DEVICE_HOSTNAME);
    ArduinoOTA.setPassword(settings.otaPass.c_str());
    ArduinoOTA.onStart([] {
        otaRunning = true;
        displayOtaProgress(0);
    });
    ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
        static int lastPct = -1;
        int pct = total ? (int)(done * 100ULL / total) : 0;
        if (pct != lastPct) {
            lastPct = pct;
            displayOtaProgress(pct);
        }
    });
    ArduinoOTA.onEnd([] { displayOtaEnd(true); });
    ArduinoOTA.onError([](ota_error_t) {
        otaRunning = false;
        displayOtaEnd(false);
    });
    ArduinoOTA.begin();
    otaStarted = true;
}

void netSetup() {
    weatherMutex = xSemaphoreCreateMutex();
    WiFi.setHostname(DEVICE_HOSTNAME);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    startWifi();
    xTaskCreatePinnedToCore(weatherTask, "weather", 8192, nullptr, 1, nullptr, 0);
}

void netLoop() {
    if (!WiFi.isConnected()) return;
    if (configuredOffset == INT32_MIN) applyTimezone(settings.utcOffset);  // fallback: zona de la PC
    if (!otaStarted && settings.otaPass.length() > 0) startOta();
    if (otaStarted) ArduinoOTA.handle();
}

static bool wifiEnabled = true;

void netSetEnabled(bool enabled) {
    if (enabled == wifiEnabled) return;
    wifiEnabled = enabled;
    if (enabled) {
        WiFi.mode(WIFI_STA);
        startWifi();
    } else {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }
}

void netApplySettings(bool wifiChanged, bool locationChanged) {
    if (wifiChanged && wifiEnabled) startWifi();
    if (otaStarted) ArduinoOTA.setPassword(settings.otaPass.c_str());
    if (wifiChanged || locationChanged) refreshRequested = true;
}

bool netWifiConnected() { return WiFi.isConnected(); }

String netIp() { return WiFi.isConnected() ? WiFi.localIP().toString() : String(""); }

bool netLocalTime(struct tm &out) {
    if (configuredOffset == INT32_MIN) return false;
    time_t now = time(nullptr);
    if (now < 1700000000) return false;  // NTP todavía no sincronizó
    localtime_r(&now, &out);
    return true;
}

bool netGetWeather(WeatherData &out) {
    if (!weatherMutex) return false;  // la pantalla puede pedir datos antes de netSetup()
    xSemaphoreTake(weatherMutex, portMAX_DELAY);
    out = weather;
    xSemaphoreGive(weatherMutex);
    return out.valid;
}

bool netConsumeWeatherUpdate() {
    if (!weatherUpdated) return false;
    weatherUpdated = false;
    return true;
}
