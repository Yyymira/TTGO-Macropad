#include "display.h"

#include "battery.h"
#include "ble_kbd.h"
#include "config.h"
#include "i18n.h"
#include "media.h"
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
static bool layerToPhone = false;  // capa con destino celular (con la PC presente)
static int8_t viewOverride = -1;   // pantalla de reposo fijada desde un botón
static bool meetActive = false, meetMic = false;  // reunión en el navegador (Meet/Teams)
static bool showingWeather = false;
static bool showingMedia = false;
// Pantallas de reposo. En modo "ambos" rotan Stats -> Clima -> Multimedia (esta
// última solo mientras suena algo); el botón del pin puede fijar cualquiera.
enum IdleScreen : uint8_t { ScreenStats = 0, ScreenWeather = 1, ScreenMedia = 2 };
static uint8_t bothPhase = ScreenStats;
static uint32_t lastPhaseMs = 0;
static int lastDrawnMinute = -1;
static bool otaActive = false;

static bool lowWarned = false;  // el cartel de batería baja se muestra una vez por descarga
static uint32_t warningUntilMs = 0;
static bool dimmed = false;


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
    if (code == 0) return tr(Str::SkyClear);
    if (code == 1) return tr(Str::SkyMostlyClear);
    if (code == 2) return tr(Str::SkyPartlyCloudy);
    if (code >= 80 && code <= 82) return tr(Str::SkyShowers);
    switch (skyOf(code)) {
        case Sky::Fog: return tr(Str::SkyFog);
        case Sky::Drizzle: return tr(Str::SkyDrizzle);
        case Sky::Rain: return tr(Str::SkyRain);
        case Sky::Snow: return tr(Str::SkySnow);
        case Sky::Storm: return tr(Str::SkyStorm);
        default: return tr(Str::SkyCloudy);
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

// Pila chica (20x9) con el nivel; "enchufada" muestra un rayo. Devuelve el ancho dibujado.
static int drawBattery(int x, int y) {
    const bool usb = batteryOnUsb();
    const int pct = batteryPercent();
    if (!usb && pct < 0) return 0;
    uint16_t color = usb ? TFT_SKYBLUE : batteryLow() ? TFT_RED : pct <= 30 ? TFT_ORANGE : TFT_GREEN;
    tft.drawRect(x, y, 18, 9, TFT_SILVER);
    tft.fillRect(x + 18, y + 2, 2, 5, TFT_SILVER);
    tft.fillRect(x + 1, y + 1, 16, 7, TFT_BLACK);
    if (usb) {
        tft.fillTriangle(x + 10, y + 1, x + 5, y + 5, x + 9, y + 5, color);
        tft.fillTriangle(x + 8, y + 7, x + 13, y + 3, x + 9, y + 3, color);
    } else {
        int w = 16 * pct / 100;
        if (w < 1) w = 1;
        tft.fillRect(x + 1, y + 1, w, 7, color);
    }
    tft.setTextFont(1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(color, TFT_BLACK);
    String txt = usb ? "USB" : String(pct) + "%";
    tft.drawString(txt, x + 23, y + 1);
    return 23 + tft.textWidth(txt);
}

// Indicador chico de la pantalla de reposo (solo con la PC): "A" si alterna
// sola entre stats y clima, un pin si quedó fija (por config o por botón).
// (x, y) es la esquina superior izquierda de un cuadro de 8x11.
static void drawViewIndicator(int x, int y) {
    if (!hostPresent) return;
    bool alternating = viewOverride < 0 && settings.idleView == IdleView::Both;
    if (alternating) {
        tft.setTextFont(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("A", x + 1, y + 2);
        return;
    }
    // Pin de 8x11 (diseño de Yyymira): R = cabeza, N = aguja, . = transparente.
    static const char *const PIN[] = {
        "RRRRRRRR", ".RRRRRR.", "..RRRR..", "..RRRR..", "..RRRR..", ".RRRRRR.",
        "RRRRRRRR", "...NN...", "...NN...", "...NN...", "...NN...",
    };
    const bool byButton = viewOverride >= 0;  // en color si se fijó con el botón, gris si es por config
    const uint16_t head = byButton ? tft.color565(200, 54, 60) : TFT_DARKGREY;
    const uint16_t needle = byButton ? tft.color565(184, 252, 252) : TFT_DARKGREY;
    for (int row = 0; row < 11; row++) {
        for (int col = 0; col < 8; col++) {
            char p = PIN[row][col];
            if (p != '.') tft.drawPixel(x + col, y + row, p == 'R' ? head : needle);
        }
    }
}

// Micrófono de la reunión: verde si está abierto, rojo y tachado si está
// silenciado. Solo aparece durante una reunión. Ocupa 9x12 desde (x, y).
static void drawMeetIndicator(int x, int y) {
    if (!meetActive || !hostPresent) return;
    uint16_t c = meetMic ? TFT_GREEN : TFT_RED;
    tft.fillRect(x - 1, y, 11, 12, TFT_BLACK);
    tft.fillRoundRect(x + 2, y, 5, 7, 2, c);  // cápsula
    tft.drawFastVLine(x, y + 3, 4, c);        // soporte en U
    tft.drawFastVLine(x + 8, y + 3, 4, c);
    tft.drawPixel(x + 1, y + 7, c);
    tft.drawPixel(x + 7, y + 7, c);
    tft.drawFastHLine(x + 2, y + 8, 5, c);
    tft.drawFastVLine(x + 4, y + 9, 2, c);    // pie
    tft.drawFastHLine(x + 2, y + 11, 5, c);
    if (!meetMic) {
        tft.drawLine(x - 1, y, x + 9, y + 11, TFT_RED);
        tft.drawLine(x, y, x + 10, y + 11, TFT_RED);
    }
}

static void renderLowBattery() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString(tr(Str::BatteryLow), 120, 50);
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String(batteryPercent()) + "% " + tr(Str::BatteryLeft), 120, 84);
    tft.setTextDatum(TL_DATUM);
}

static void renderWeather() {
    showingWeather = true;
    showingMedia = false;
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
            tft.drawString(tr(Str::SetupWifiCity1), 120, 58);
            tft.drawString(tr(Str::SetupWifiCity2), 120, 78);
        } else if (!netWifiConnected()) {
            tft.drawString(tr(Str::ConnectingWifi), 120, 58);
            tft.setTextColor(TFT_SILVER, TFT_BLACK);
            tft.drawString(settings.wifiSsid, 120, 78);
        } else if (isnan(settings.lat)) {
            tft.drawString(tr(Str::MissingCity), 120, 67);
        } else {
            tft.drawString(tr(Str::FetchingWeather), 120, 67);
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
        int d1 = dayFirstDates() ? t.tm_mday : t.tm_mon + 1, d2 = dayFirstDates() ? t.tm_mon + 1 : t.tm_mday;
        snprintf(date, sizeof date, "%s %02d/%02d", dayName(t.tm_wday), d1, d2);
        tft.setTextFont(2);
        tft.setTextColor(TFT_SILVER, TFT_BLACK);
        tft.drawString(date, 6, 58);
        int afterDate = 6 + tft.textWidth(date, 2) + 10;
        if (!hostPresent) {
            drawBattery(afterDate, 61);
        } else {
            drawViewIndicator(afterDate, 60);
        }
    }

    if (!hasWeather) {
        tft.setTextFont(2);
        tft.setTextColor(TFT_SILVER, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(tr(Str::NoWeather), 120, 104);
        tft.setTextDatum(TL_DATUM);
        return;
    }

    drawMeetIndicator(228, 1);

    // Actual: ícono + temperatura + humedad, a la derecha de la hora.
    drawWeatherIcon(164, 20, 13, w.code);
    drawDegrees(String((int)lroundf(w.temp)), 184, 6, 4, TFT_WHITE);
    tft.setTextFont(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(String(tr(Str::Humidity)) + " " + String(w.humidity) + "%", 184, 34);
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
        tft.drawString(i == 0 ? tr(Str::Today) : dayName(w.days[i].wday), cx, 80);
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
    showingMedia = false;
    tft.fillScreen(TFT_BLACK);

    if (statCpu < 0) {
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(2);
        tft.drawString(tr(Str::WaitingData), 8, 60);
        return;
    }

    drawGauge(58, 46, 40, 30, statCpu, "CPU");
    drawGauge(182, 46, 40, 30, statGpu, "GPU");
    drawViewIndicator(116, 2);  // arriba al centro, entre los dos gauges
    drawMeetIndicator(228, 1);

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

static IdleScreen idleScreen() {
    if (!hostPresent) return ScreenWeather;
    if (viewOverride >= 0) return static_cast<IdleScreen>(viewOverride);
    switch (settings.idleView) {
        case IdleView::Stats: return ScreenStats;
        case IdleView::Weather: return ScreenWeather;
        default: return static_cast<IdleScreen>(bothPhase);
    }
}

static uint16_t phaseSeconds(uint8_t phase) {
    return phase == ScreenStats ? settings.statsSec : phase == ScreenWeather ? settings.weatherSec : settings.mediaSec;
}

// ------------------------------ multimedia ----------------------------

static String fitText(String s, int font, int maxW) {
    if (tft.textWidth(s, font) <= maxW) return s;
    while (s.length() > 0 && tft.textWidth(s + "...", font) > maxW) s.remove(s.length() - 1);
    return s + "...";
}

static String clock(int32_t secs) {
    if (secs < 0) secs = 0;
    char buf[12];
    if (secs >= 3600) {
        snprintf(buf, sizeof buf, "%d:%02d:%02d", (int)(secs / 3600), (int)(secs / 60 % 60), (int)(secs % 60));
    } else {
        snprintf(buf, sizeof buf, "%d:%02d", (int)(secs / 60), (int)(secs % 60));
    }
    return buf;
}

static const int MEDIA_TX = 110, MEDIA_TW = 126;  // columna de texto a la derecha de la tapa

// Barra de progreso, tiempos y el botón de play/pausa: lo que cambia cada segundo.
static void drawMediaProgress() {
    const MediaInfo &m = mediaInfo();
    tft.fillRect(0, 76, 240, 59, TFT_BLACK);

    // Controles (solo muestran el estado; se manejan con las teclas)
    const int cy = 88;
    uint16_t c = TFT_SILVER;
    tft.fillTriangle(MEDIA_TX + 20, cy - 6, MEDIA_TX + 20, cy + 6, MEDIA_TX + 12, cy, c);  // anterior
    tft.fillRect(MEDIA_TX + 10, cy - 6, 2, 13, c);
    int px = MEDIA_TX + 58;
    if (m.playing) {  // pausa
        tft.fillRect(px - 6, cy - 7, 4, 15, TFT_WHITE);
        tft.fillRect(px + 2, cy - 7, 4, 15, TFT_WHITE);
    } else {  // play
        tft.fillTriangle(px - 5, cy - 8, px - 5, cy + 8, px + 7, cy, TFT_WHITE);
    }
    tft.fillTriangle(MEDIA_TX + 96, cy - 6, MEDIA_TX + 96, cy + 6, MEDIA_TX + 104, cy, c);  // siguiente
    tft.fillRect(MEDIA_TX + 104, cy - 6, 2, 13, c);

    // Progreso
    int pct = m.len > 0 ? constrain(m.pos * 100 / m.len, 0, 100) : 0;
    drawBar(6, 109, 228, 7, pct, TFT_GREEN);
    tft.setTextFont(1);
    tft.setTextColor(TFT_SILVER, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(clock(m.pos), 6, 122);
    if (m.len > 0) {
        tft.setTextDatum(TR_DATUM);
        tft.drawString(clock(m.len), 234, 122);
    }
    tft.setTextDatum(TL_DATUM);
}

static void renderMedia() {
    showingWeather = false;
    showingMedia = true;
    tft.fillScreen(TFT_BLACK);
    const MediaInfo &m = mediaInfo();

    // Tapa, o una nota musical si no hay
    if (const uint16_t *art = mediaArt()) {
        bool swap = tft.getSwapBytes();
        tft.setSwapBytes(true);
        tft.pushImage(6, 6, ART_SIZE, ART_SIZE, art);
        tft.setSwapBytes(swap);
    } else {
        tft.fillRoundRect(6, 6, ART_SIZE, ART_SIZE, 6, tft.color565(40, 40, 48));
        tft.fillCircle(46, 70, 9, TFT_DARKGREY);
        tft.fillRect(53, 28, 4, 43, TFT_DARKGREY);
        tft.fillTriangle(57, 28, 57, 42, 70, 36, TFT_DARKGREY);
    }

    tft.setTextDatum(TL_DATUM);
    if (!m.active) {
        tft.setTextFont(2);
        tft.setTextColor(TFT_SILVER, TFT_BLACK);
        tft.drawString(fitText(tr(Str::NothingPlaying), 2, MEDIA_TW), MEDIA_TX, 40);
        drawViewIndicator(228, 2);
        return;
    }

    // Título en hasta 2 líneas, cortando entre palabras
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    String title = m.title, line1 = title, line2 = "";
    if (tft.textWidth(title, 2) > MEDIA_TW) {
        int cut = title.length();
        while (cut > 0 && tft.textWidth(title.substring(0, cut), 2) > MEDIA_TW) {
            int sp = title.lastIndexOf(' ', cut - 1);
            cut = sp > 0 ? sp : cut - 1;
        }
        line1 = title.substring(0, cut);
        line2 = title.substring(cut);
        line2.trim();
    }
    tft.drawString(fitText(line1, 2, MEDIA_TW), MEDIA_TX, 6);
    if (line2.length()) tft.drawString(fitText(line2, 2, MEDIA_TW), MEDIA_TX, 22);

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(fitText(m.artist, 2, MEDIA_TW), MEDIA_TX, 42);
    tft.setTextFont(1);
    tft.setTextColor(TFT_SILVER, TFT_BLACK);
    tft.drawString(fitText(m.album, 1, MEDIA_TW), MEDIA_TX, 62);

    drawMediaProgress();
    drawViewIndicator(116, 120);  // abajo al centro, entre los tiempos
    drawMeetIndicator(130, 120);
}

// Etiqueta centrada en su celda; si no entra con la fuente normal, usa la chica.
static void drawCell(const String &label, int x, int y, int w, int h) {
    tft.drawRect(x, y, w, h, TFT_DARKGREY);
    tft.setTextFont(tft.textWidth(label, 2) <= w - 4 ? 2 : 1);
    tft.drawString(label, x + w / 2, y + h / 2);
}

static void renderLayer() {
    showingWeather = false;
    showingMedia = false;
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString(String(tr(Str::Layer)) + " " + String(layerIndex) + ": " + layerName, 8, 4);
    drawMeetIndicator(228, 4);

    if (!hostPresent || layerToPhone) {  // teclas al celular: batería y si hay uno conectado
        drawBattery(170, 7);
        tft.setTextDatum(TR_DATUM);
        tft.setTextFont(2);
        tft.setTextColor(bleConnected() ? TFT_SKYBLUE : TFT_DARKGREY, TFT_BLACK);
        tft.drawString(bleConnected() ? "BT" : "BT --", 166, 4);
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
    if (millis() < warningUntilMs) {
        renderLowBattery();
        return;
    }
    if (currentMode == DisplayMode::Layer) {
        renderLayer();
    } else {
        switch (idleScreen()) {
            case ScreenWeather: renderWeather(); break;
            case ScreenMedia: renderMedia(); break;
            default: renderStats(); break;
        }
    }
}

// ------------------------------- API ----------------------------------

void displaySetup() {
    tft.init();
    tft.setRotation(1);  // landscape, 240x135
    // Backlight por PWM para poder bajarlo con batería crítica.
    ledcSetup(BACKLIGHT_CHANNEL, 5000, 8);
    ledcAttachPin(TFT_BL, BACKLIGHT_CHANNEL);
    ledcWrite(BACKLIGHT_CHANNEL, 255);
    tft.fillScreen(TFT_BLACK);
    render();
}

void displayLoop() {
    if (otaActive) return;
    uint32_t now = millis();

    // Modo "ambos": Stats -> Clima -> Multimedia (si suena algo). Si la música
    // se detiene mientras se ve la multimedia, pasa enseguida a la siguiente.
    if (hostPresent && viewOverride < 0 && settings.idleView == IdleView::Both) {
        bool mediaGone = bothPhase == ScreenMedia && !mediaPlaying();
        if (mediaGone || now - lastPhaseMs >= 1000UL * phaseSeconds(bothPhase)) {
            lastPhaseMs = now;
            bothPhase = (bothPhase + 1) % 3;
            if (bothPhase == ScreenMedia && !mediaPlaying()) bothPhase = ScreenStats;
            if (currentMode == DisplayMode::Idle) render();
        }
    }

    bool trackChanged = false;
    if (mediaConsumeChange(trackChanged) && currentMode == DisplayMode::Idle && showingMedia) {
        if (trackChanged) {
            render();
        } else {
            drawMediaProgress();
            drawViewIndicator(116, 120);
            drawMeetIndicator(130, 120);
        }
    }

    if (currentMode == DisplayMode::Idle && showingWeather) {
        struct tm t;
        int minute = netLocalTime(t) ? t.tm_min : -1;
        if (minute != lastDrawnMinute || netConsumeWeatherUpdate()) render();
    }

    // Batería: cartel una vez al cruzar el umbral, brillo bajo en crítico.
    if (batteryConsumeChange()) {
        if (batteryLow() && !lowWarned) {
            lowWarned = true;
            warningUntilMs = now + 4000;
        }
        if (!batteryLow()) lowWarned = false;
        bool dim = batteryCritical();
        if (dim != dimmed) {
            dimmed = dim;
            ledcWrite(BACKLIGHT_CHANNEL, dim ? BACKLIGHT_DIM : 255);
        }
        if (!hostPresent && (currentMode == DisplayMode::Layer || showingWeather)) render();
    }
    if (warningUntilMs && now >= warningUntilMs) {
        warningUntilMs = 0;
        render();
    }

    static bool lastBle = false;
    bool ble = bleConnected();
    if (ble != lastBle) {
        lastBle = ble;
        if (currentMode == DisplayMode::Layer && (!hostPresent || layerToPhone)) render();
    }
}

void displaySetViewOverride(int8_t view) {
    if (view == viewOverride) return;
    viewOverride = view;
    bothPhase = 0;
    lastPhaseMs = millis();
    if (currentMode == DisplayMode::Idle) render();
}

void displaySetMeeting(bool active, bool mic) {
    if (active == meetActive && mic == meetMic) return;
    meetActive = active;
    meetMic = mic;
    render();
}

void displaySetLayerToPhone(bool toPhone) { layerToPhone = toPhone; }  // se dibuja con displaySetLayer

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
    if (currentMode == DisplayMode::Idle && idleScreen() == ScreenStats) render();
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
        tft.drawString(tr(Str::Updating), 120, 42);
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
    tft.drawString(ok ? tr(Str::UpdateDone) : tr(Str::UpdateError), 120, 67);
    tft.setTextDatum(TL_DATUM);
    if (!ok) {
        delay(2000);
        otaActive = false;
        render();
    }
}
