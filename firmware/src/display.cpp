#include "display.h"
#include "config.h"

#include <TFT_eSPI.h>

static TFT_eSPI tft = TFT_eSPI();
static DisplayMode currentMode = DisplayMode::Idle;

static int statCpu = -1, statRam = -1, statCpuTemp = -1, statGpu = -1, statGpuTemp = -1;
static uint8_t layerIndex = 0;
static String layerName = "";
static String layerLabels[NUM_BUTTONS];

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

static void renderIdle() {
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
    tft.drawString("RAM", 4, 98);
    drawBar(38, 96, 108, 14, statRam, TFT_CYAN);
    tft.drawString(String(statRam) + "%", 150, 98);

    tft.drawString("CPU " + String(statCpuTemp) + "C", 182, 96);
    tft.drawString("GPU " + String(statGpuTemp) + "C", 182, 114);
}

static void renderLayer() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString("Capa " + String(layerIndex) + ": " + layerName, 8, 4);

    // Grilla 3x2 para los Cherry MX (misma disposición que en el gabinete) y
    // una columna angosta a la derecha para los 2 botones integrados, que en
    // el gabinete quedan a la derecha del display.
    const int top = 24;
    const int sideW = 54;
    const int cols = 3, rows = 2;
    const int cellW = (240 - sideW) / cols, cellH = (135 - top) / rows;

    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    for (uint8_t i = 0; i < 6; i++) {
        int x = (i % cols) * cellW, y = top + (i / cols) * cellH;
        tft.drawRect(x, y, cellW, cellH, TFT_DARKGREY);
        tft.drawString(layerLabels[i], x + cellW / 2, y + cellH / 2);
    }

    tft.setTextColor(TFT_SILVER, TFT_BLACK);
    for (uint8_t i = 6; i < NUM_BUTTONS; i++) {
        int x = cols * cellW, y = top + (i - 6) * cellH;
        tft.drawRect(x, y, 240 - x, cellH, TFT_DARKGREY);
        tft.drawString(layerLabels[i], x + (240 - x) / 2, y + cellH / 2);
    }
    tft.setTextDatum(TL_DATUM);
}

static void render() {
    if (currentMode == DisplayMode::Idle) {
        renderIdle();
    } else {
        renderLayer();
    }
}

void displaySetup() {
    tft.init();
    tft.setRotation(1);  // landscape, 240x135
    tft.fillScreen(TFT_BLACK);
    render();
}

void displaySetMode(DisplayMode mode) {
    if (mode == currentMode) return;
    currentMode = mode;
    render();
}

void displaySetStats(int cpu, int ram, int cpuTemp, int gpu, int gpuTemp) {
    statCpu = cpu;
    statRam = ram;
    statCpuTemp = cpuTemp;
    statGpu = gpu;
    statGpuTemp = gpuTemp;
    if (currentMode == DisplayMode::Idle) render();
}

void displaySetLayer(uint8_t index, const String &name, const String labels[NUM_BUTTONS]) {
    layerIndex = index;
    layerName = name;
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) layerLabels[i] = labels[i];
    if (currentMode == DisplayMode::Layer) render();
}
