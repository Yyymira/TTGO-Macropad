#pragma once

#include <Arduino.h>
#include <time.h>

struct WeatherDay {
    int code = 0;
    float tmax = 0;
    float tmin = 0;
    uint8_t wday = 0;  // 0 = domingo
};

struct WeatherData {
    bool valid = false;
    float temp = 0;
    int humidity = 0;
    int code = 0;
    WeatherDay days[3];
};

// WiFi + hora (NTP) + clima (Open-Meteo, en una tarea aparte para no
// trabar los botones) + actualización OTA.
void netSetup();
void netLoop();
void netApplySettings(bool wifiChanged, bool locationChanged);

bool netWifiConnected();
String netIp();
bool netLocalTime(struct tm &out);
bool netGetWeather(WeatherData &out);
bool netConsumeWeatherUpdate();
// Ahorro de energía con batería crítica: apaga el WiFi y lo vuelve a prender.
void netSetEnabled(bool enabled);
