#include "display.h"

#include "ble_kbd.h"
#include "config.h"
#include "net.h"
#include "settings.h"

#include <TFT_eSPI.h>

static TFT_eSPI tft = TFT_eSPI();
static DisplayMode currentMode = DisplayMode::Idle;

static int statCpu = -1, statRam = -1, statCpuTemp = -1, statGpu = -1, statGpuTemp = -1, statVram = -1;
static uint8_t layerIndex = 0;
static String layerName = "";
static String layerLabels[NUM_BUTTONS];

static bool hostPresent = false;
static bool showingWeather = false;
static uint8_t bothPhase = 0;  // modo "ambos": 0 = stats, 1 = clima
static uint32_t lastPhaseMs = 0;
static int lastDrawnMinute = -1;
static bool otaActive = false;

static const char *DAY_NAMES[] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};

static void drawBar(int x, int y, int w, int h, int pct, uint16_t color) {
    pct = constrain(pct, 0, 100);
    tft.drawRect(x, y, w, h, TFT_DARKGREY);
    int fillW = (w - 2) * pct / 100;
    tft.fillRect(x + 1, y + 1, w - 2, h - 2, TFT_BLACK);
    if (fillW > 0) tft.fillRect(x + 1, y + 1, fillW, h - 2, color);
}

static uint16_t gaugeColor(int pct) {
    if (pct < 60) return TFT_GREEN;
    if (pct < 85) return TFT_ORANGE;
    return TFT_RED;
}

// Gauge tipo velocímetro: arco de 270° abierto hacia abajo que se llena de
// izquierda a derecha. En TFT_eSPI los ángulos de drawArc se miden en sentido
// horario desde las 6 en punto, así que 45° es abajo a la izquierda y 315°
// abajo a la derecha.
static void drawGauge(int cx, int cy, int r, int ir, int pct, const char *label) {
    pct = constrain(pct, 0, 100);
    const uint32_t trackStart = 45, trackEnd = 315;
    tft.drawArc(cx, cy, r, ir, trackStart, trackEnd, TFT_DARKGREY, TFT_BLACK, true);

    if (pct > 0) {
        uint32_t valueEnd = trackStart + (uint32_t)(pct * 270 / 100);
        tft.drawArc(cx, cy, r, ir, trackStart, valueEnd, gaugeColor(pct), TFT_BLACK, true);
    }

    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String(pct) + "%", cx, cy - 4);
    tft.setTextFont(2);
    tft.setTextColor(TFT_SILVER, TFT_BLACK);
    tft.drawString(label, cx, cy + 22);
    tft.setTextDatum(TL_DATUM);
}

// ------------------------------- clima -------------------------------

enum class Sky { Clear, Partly, Cloudy, Fog, Drizzle, Rain, Snow, Storm };

// Códigos WMO que devuelve Open-Meteo.
static Sky skyOf(int code) {
    if (code == 0) return Sky::Clear;
    if (code <= 2) return Sky::Partly;
    if (code == 3) return Sky::Cloudy;
    if (code == 45 || code == 48) return Sky::Fog;
    if (code >= 51 && code <= 57) return Sky::Drizzle;
    if ((code >= 61 && code <= 67) || (code >= 80 && code <= 82)) return Sky::Rain;
    if ((code >= 71 && code <= 77) || code == 85 || code == 86) return Sky::Snow;
    if (code >= 95) return Sky::Storm;
    return Sky::Cloudy;
}

static const char *skyText(int code) {
    if (code == 0) return "Despejado";
    if (code == 1) return "Casi despejado";
    if (code == 2) return "Algo nublado";
    if (code >= 80 && code <= 82) return "Chaparrones";
    switch (skyOf(code)) {
        case Sky::Fog: return "Niebla";
        case Sky::Drizzle: return "Llovizna";
        case Sky::Rain: return "Lluvia";
        case Sky::Snow: return "Nieve";
        case Sky::Storm: return "Tormenta";
        default: return "Nublado";
    }
}

static void drawSun(int cx, int cy, int s) {
    int r = s * 5 / 10;
    for (int a = 0; a < 360; a += 45) {
        float rad = a * DEG_TO_RAD;
        tft.drawLine(cx + cosf(rad) * (r + 2), cy + sinf(rad) * (r + 2), cx + cosf(rad) * (r + s / 3 + 2),
                     cy + sinf(rad) * (r + s / 3 + 2), TFT_YELLOW);
    }
    tft.fillCircle(cx, cy, r, TFT_YELLOW);
}

static void drawCloud(int cx, int cy, int s, uint16_t color) {
    tft.fillCircle(cx - s * 4 / 10, cy + s / 10, s * 4 / 10, color);
    tft.fillCircle(cx + s / 10, cy - s * 2 / 10, s * 55 / 100, color);
    tft.fillCircle(cx + s * 55 / 100, cy + s * 15 / 100, s * 35 / 100, color);
    tft.fillRect(cx - s * 4 / 10, cy + s / 10, s * 95 / 100, s * 4 / 10, color);
}

// Ícono centrado en (cx, cy), de unos 2*s píxeles de lado.
static void drawWeatherIcon(int cx, int cy, int s, int code) {
    Sky sky = skyOf(code);
    switch (sky) {
        case Sky::Clear:
            drawSun(cx, cy, s);
            break;
        case Sky::Partly:
            drawSun(cx - s * 3 / 10, cy - s * 3 / 10, s * 7 / 10);
            drawCloud(cx + s / 10, cy + s * 2 / 10, s * 8 / 10, TFT_LIGHTGREY);
            break;
        case Sky::Cloudy:
            drawCloud(cx, cy, s, TFT_LIGHTGREY);
            break;
        case Sky::Fog:
            for (int i = -1; i <= 1; i++) tft.fillRect(cx - s, cy + i * s / 2 - 1, 2 * s, 3, TFT_LIGHTGREY);
            break;
        case Sky::Drizzle:
        case Sky::Rain: {
            drawCloud(cx, cy - s / 4, s, TFT_LIGHTGREY);
            int drops = sky == Sky::Rain ? 3 : 2;
            for (int i = 0; i < drops; i++) {
                int x = cx - s / 2 + i * s / (drops - 1 > 0 ? drops - 1 : 1);
                tft.drawLine(x, cy + s * 4 / 10, x - 3, cy + s * 9 / 10, TFT_SKYBLUE);
            }
            break;
        }
        case Sky::Snow:
            drawCloud(cx, cy - s / 4, s, TFT_LIGHTGREY);
            for (int i = -1; i <= 1; i++) tft.fillCircle(cx + i * s / 2, cy + s * 7 / 10, 2, TFT_WHITE);
            break;
        case Sky::Storm:
            drawCloud(cx, cy - s / 4, s, TFT_DARKGREY);
            tft.fillTriangle(cx, cy + s / 4, cx - s / 3, cy + s * 7 / 10, cx + s / 10, cy + s * 6 / 10, TFT_YELLOW);
            tft.fillTriangle(cx + s / 10, cy + s * 5 / 10, cx - s / 10, cy + s, cx + s / 3, cy + s * 5 / 10, TFT_YELLOW);
            break;
    }
}

static void drawDegrees(const String &value, int x, int y, int font, uint16_t color) {
    tft.setTextFont(font);
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(value, x, y);
    int w = tft.textWidth(value);
    tft.drawCircle(x + w + 3, y + 3, 2, color);
}

static void renderWeather() {
    showingWeather = true;
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);

    struct tm t;
    bool hasTime = netLocalTime(t);
    WeatherData w;
    bool hasWeather = netGetWeather(w);
    lastDrawnMinute = hasTime ? t.tm_min : -1;

    if (!hasTime && !hasWeather) {
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if (settings.wifiSsid.isEmpty()) {
            tft.drawString("Configura WiFi y ciudad", 120, 58);
            tft.drawString("desde el panel de la PC", 120, 78);
        } else if (!netWifiConnected()) {
            tft.drawString("Conectando a WiFi...", 120, 58);
            tft.setTextColor(TFT_SILVER, TFT_BLACK);
            tft.drawString(settings.wifiSsid, 120, 78);
        } else if (isnan(settings.lat)) {
            tft.drawString("Falta configurar la ciudad", 120, 67);
        } else {
            tft.drawString("Buscando hora y clima...", 120, 67);
        }
        tft.setTextDatum(TL_DATUM);
        return;
    }

    if (hasTime) {
        char hhmm[6];
        snprintf(hhmm, sizeof hhmm, "%02d:%02d", t.tm_hour, t.tm_min);
        tft.setTextFont(7);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString(hhmm, 2, 4);
        char date[16];
        snprintf(date, sizeof date, "%s %02d/%02d", DAY_NAMES[t.tm_wday], t.tm_mday, t.tm_mon + 1);
        tft.setTextFont(2);
        tft.setTextColor(TFT_SILVER, TFT_BLACK);
        tft.drawString(date, 6, 58);
    }

    if (!hasWeather) {
        tft.setTextFont(2);
        tft.setTextColor(TFT_SILVER, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Sin datos del clima", 120, 104);
        tft.setTextDatum(TL_DATUM);
        return;
    }

    // Actual: ícono + temperatura + humedad, a la derecha de la hora.
    drawWeatherIcon(164, 20, 13, w.code);
    drawDegrees(String((int)lroundf(w.temp)), 184, 6, 4, TFT_WHITE);
    tft.setTextFont(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("Hum " + String(w.humidity) + "%", 184, 34);
    tft.setTextColor(TFT_SILVER, TFT_BLACK);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(skyText(w.code), 236, 58);

    // Pronóstico: hoy y los 2 días siguientes.
    tft.drawFastHLine(0, 77, 240, TFT_DARKGREY);
    tft.setTextDatum(TC_DATUM);
    for (int i = 0; i < 3; i++) {
        int cx = i * 80 + 40;
        if (i > 0) tft.drawFastVLine(i * 80, 80, 52, TFT_DARKGREY);
        tft.setTextFont(2);
        tft.setTextColor(TFT_SILVER, TFT_BLACK);
        tft.drawString(i == 0 ? "Hoy" : DAY_NAMES[w.days[i].wday], cx, 80);
        drawWeatherIcon(cx, 104, 9, w.days[i].code);
        String range = String((int)lroundf(w.days[i].tmax)) + " / " + String((int)lroundf(w.days[i].tmin));
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString(range, cx, 118);
    }
    tft.setTextDatum(TL_DATUM);
}

// ------------------------------- stats --------------------------------

static void renderStats() {
    showingWeather = false;
    tft.fillScreen(TFT_BLACK);

    if (statCpu < 0) {
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(2);
        tft.drawString("Esperando datos...", 8, 60);
        return;
    }

    drawGauge(58, 46, 40, 30, statCpu, "CPU");
    drawGauge(182, 46, 40, 30, statGpu, "GPU");

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString("RAM", 4, 94);
    drawBar(44, 94, 100, 14, statRam, TFT_CYAN);
    tft.drawString(String(statRam) + "%", 148, 94);

    tft.drawString("VRAM", 4, 114);
    drawBar(44, 114, 100, 14, statVram, TFT_MAGENTA);
    tft.drawString(statVram < 0 ? String("--") : String(statVram) + "%", 148, 114);

    tft.drawString("CPU " + String(statCpuTemp) + "C", 184, 94);
    tft.drawString("GPU " + String(statGpuTemp) + "C", 184, 114);
}

static bool idleShowsWeather() {
    if (!hostPresent) return true;
    switch (settings.idleView) {
        case IdleView::Stats: return false;
        case IdleView::Weather: return true;
        default: return bothPhase == 1;
    }
}

// Etiqueta centrada en su celda; si no entra con la fuente normal, usa la chica.
static void drawCell(const String &label, int x, int y, int w, int h) {
    tft.drawRect(x, y, w, h, TFT_DARKGREY);
    tft.setTextFont(tft.textWidth(label, 2) <= w - 4 ? 2 : 1);
    tft.drawString(label, x + w / 2, y + h / 2);
}

static void renderLayer() {
    showingWeather = false;
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString("Capa " + String(layerIndex) + ": " + layerName, 8, 4);

    if (!hostPresent) {  // modo Bluetooth: indicar si hay un celular conectado
        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(bleConnected() ? TFT_SKYBLUE : TFT_DARKGREY, TFT_BLACK);
        tft.drawString(bleConnected() ? "BT" : "BT --", 234, 4);
    }

    // Grilla 3x2 para los Cherry MX (misma disposición que en el gabinete),
    // una columna angosta a la derecha para los 2 botones integrados y, solo
    // si la capa les asigna algo, otra a la izquierda para los pulsadores de
    // los encoders (en el gabinete, los encoders quedan a la izquierda).
    const int top = 24;
    const int sideW = 54;
    const bool encCol = layerLabels[8].length() > 0 || layerLabels[9].length() > 0;
    const int encW = encCol ? 44 : 0;
    const int cols = 3;
    const int cellW = (240 - sideW - encW) / cols, cellH = (135 - top) / 2;
    const int sideX = encW + cols * cellW;

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    for (uint8_t i = 0; i < 6; i++) {
        int x = encW + (i % cols) * cellW, y = top + (i / cols) * cellH;
        drawCell(layerLabels[i], x, y, cellW, cellH);
    }

    tft.setTextColor(TFT_SILVER, TFT_BLACK);
    for (uint8_t i = 6; i < 8; i++) drawCell(layerLabels[i], sideX, top + (i - 6) * cellH, 240 - sideX, cellH);
    if (encCol) {
        for (uint8_t i = 8; i < 10; i++) drawCell(layerLabels[i], 0, top + (i - 8) * cellH, encW, cellH);
    }
    tft.setTextDatum(TL_DATUM);
}

static void render() {
    if (otaActive) return;
    if (currentMode == DisplayMode::Layer) {
        renderLayer();
    } else if (idleShowsWeather()) {
        renderWeather();
    } else {
        renderStats();
    }
}

// ------------------------------- API ----------------------------------

void displaySetup() {
    tft.init();
    tft.setRotation(1);  // landscape, 240x135
    tft.fillScreen(TFT_BLACK);
    render();
}

void displayLoop() {
    if (otaActive) return;
    uint32_t now = millis();

    if (hostPresent && settings.idleView == IdleView::Both && now - lastPhaseMs >= IDLE_ALTERNATE_MS) {
        lastPhaseMs = now;
        bothPhase ^= 1;
        if (currentMode == DisplayMode::Idle) render();
    }

    if (currentMode == DisplayMode::Idle && showingWeather) {
        struct tm t;
        int minute = netLocalTime(t) ? t.tm_min : -1;
        if (minute != lastDrawnMinute || netConsumeWeatherUpdate()) render();
    }

    static bool lastBle = false;
    bool ble = bleConnected();
    if (ble != lastBle) {
        lastBle = ble;
        if (currentMode == DisplayMode::Layer && !hostPresent) render();
    }
}

void displaySetHostPresent(bool present) {
    if (present == hostPresent) return;
    hostPresent = present;
    bothPhase = 0;
    lastPhaseMs = millis();
    render();
}

void displayRefresh() { render(); }

DisplayMode displayGetMode() { return currentMode; }

void displaySetMode(DisplayMode mode) {
    if (mode == currentMode) return;
    currentMode = mode;
    render();
}

void displaySetStats(int cpu, int ram, int cpuTemp, int gpu, int gpuTemp, int vram) {
    statCpu = cpu;
    statRam = ram;
    statCpuTemp = cpuTemp;
    statGpu = gpu;
    statGpuTemp = gpuTemp;
    statVram = vram;
    if (currentMode == DisplayMode::Idle && !idleShowsWeather()) render();
}

void displaySetLayer(uint8_t index, const String &name, const String labels[NUM_BUTTONS]) {
    layerIndex = index;
    layerName = name;
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) layerLabels[i] = labels[i];
    if (currentMode == DisplayMode::Layer) render();
}

void displayOtaProgress(int pct) {
    if (!otaActive) {
        otaActive = true;
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("Actualizando...", 120, 42);
    }
    drawBar(20, 70, 200, 18, pct, TFT_GREEN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(2);
    tft.setTextColor(TFT_SILVER, TFT_BLACK);
    tft.setTextPadding(60);
    tft.drawString(String(pct) + "%", 120, 104);
    tft.setTextPadding(0);
    tft.setTextDatum(TL_DATUM);
}

void displayOtaEnd(bool ok) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4);
    tft.setTextColor(ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.drawString(ok ? "Listo, reiniciando" : "Error en la subida", 120, 67);
    tft.setTextDatum(TL_DATUM);
    if (!ok) {
        delay(2000);
        otaActive = false;
        render();
    }
}
