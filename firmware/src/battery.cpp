#include "battery.h"

#include "config.h"

static float volts = 0;
static bool onUsb = true;
static int percent = -1;
static bool changed = false;
static uint32_t lastSampleMs = 0;

// Curva de descarga típica de una LiPo de 1 celda (voltaje en reposo).
static const float CURVE_V[] = {3.30, 3.45, 3.55, 3.65, 3.70, 3.75, 3.80, 3.90, 4.00, 4.10, 4.20};
static const int CURVE_P[] = {0, 5, 10, 20, 30, 40, 50, 65, 80, 90, 100};
static constexpr int CURVE_N = sizeof(CURVE_P) / sizeof(CURVE_P[0]);

static int voltsToPercent(float v) {
    if (v <= CURVE_V[0]) return 0;
    for (int i = 1; i < CURVE_N; i++) {
        if (v <= CURVE_V[i]) {
            float f = (v - CURVE_V[i - 1]) / (CURVE_V[i] - CURVE_V[i - 1]);
            return CURVE_P[i - 1] + (int)lroundf(f * (CURVE_P[i] - CURVE_P[i - 1]));
        }
    }
    return 100;
}

static float readVolts() {
    uint32_t mv = 0;
    for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(BATTERY_ADC_PIN);
    return mv / 16.0f * 2.0f / 1000.0f;  // divisor 1/2
}

void batterySetup() {
    pinMode(BATTERY_ADC_EN_PIN, OUTPUT);
    digitalWrite(BATTERY_ADC_EN_PIN, HIGH);
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
    delay(5);
    volts = readVolts();
    lastSampleMs = millis();
}

void batteryLoop() {
    if (millis() - lastSampleMs < BATTERY_SAMPLE_MS) return;
    lastSampleMs = millis();

    volts = volts * 0.8f + readVolts() * 0.2f;  // suavizado: el ADC del ESP32 es ruidoso

    // Con histéresis para que no titile en el borde.
    bool usb = onUsb ? volts > BATTERY_USB_VOLTS - 0.05f : volts > BATTERY_USB_VOLTS;
    int pct = usb ? -1 : voltsToPercent(volts);
    // A batería el porcentaje solo baja: evita que suba y baje por ruido.
    if (!usb && !onUsb && percent >= 0 && pct > percent && pct - percent < 3) pct = percent;

    if (usb != onUsb || pct != percent) changed = true;
    onUsb = usb;
    percent = pct;
}

bool batteryOnUsb() { return onUsb; }

int batteryPercent() { return percent; }

bool batteryLow() { return !onUsb && percent >= 0 && percent <= BATTERY_LOW_PCT; }

bool batteryCritical() { return !onUsb && percent >= 0 && percent <= BATTERY_CRITICAL_PCT; }

bool batteryConsumeChange() {
    bool c = changed;
    changed = false;
    return c;
}
