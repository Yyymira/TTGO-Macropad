#pragma once

#include <Arduino.h>

// Textos de la pantalla en cada idioma. La PC manda el idioma de su sistema
// en el mensaje "settings" y queda guardado, así que sin PC (Bluetooth) se
// sigue usando el último. Para sumar un idioma: agregarlo a Lang, a
// LANG_CODES y una columna más en cada fila de STRINGS.
enum class Lang : uint8_t { En = 0, Es = 1, Count };

enum class Str : uint8_t {
    SetupWifiCity1, SetupWifiCity2, ConnectingWifi, MissingCity, FetchingWeather, NoWeather,
    Humidity, Today, WaitingData, Layer, LayerNext, LayerPrev,
    BatteryLow, BatteryLeft,
    Updating, UpdateDone, UpdateError,
    SkyClear, SkyMostlyClear, SkyPartlyCloudy, SkyCloudy, SkyFog, SkyDrizzle, SkyRain, SkyShowers, SkySnow,
    SkyStorm, NothingPlaying,
    Count
};

Lang langFromCode(const char *code);  // "es", "es_AR", "en-US"... (desconocido = inglés)
const char *tr(Str s);
const char *dayName(int wday);  // 0 = domingo
bool dayFirstDates();           // 21/10 (true) o 10/21 (false)
