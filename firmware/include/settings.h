#pragma once

#include <Arduino.h>
#include <math.h>

// Pantalla de reposo cuando la PC está conectada (sin PC siempre es clima).
enum class IdleView : uint8_t { Stats = 0, Weather = 1, Both = 2 };

// Configuración que manda la PC y queda guardada en la flash (NVS), para
// que la placa funcione sola aunque arranque sin PC.
struct DeviceSettings {
    String wifiSsid;
    String wifiPass;
    String otaPass;
    float lat = NAN;
    float lon = NAN;
    int32_t utcOffset = INT32_MIN;  // segundos; INT32_MIN = desconocido
    IdleView idleView = IdleView::Both;
    uint16_t idleTimeoutSec = 15;
};

extern DeviceSettings settings;

void settingsLoad();
void settingsSave();

// Capas/mapeos para el modo Bluetooth, como JSON compacto.
String keymapLoad();
void keymapSave(const String &json);
