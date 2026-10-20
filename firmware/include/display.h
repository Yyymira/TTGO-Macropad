#pragma once

#include <Arduino.h>

#include "config.h"

enum class DisplayMode : uint8_t { Idle, Layer };

void displaySetup();
void displayLoop();  // alternancia stats/clima, reloj, indicador BT
void displaySetMode(DisplayMode mode);
DisplayMode displayGetMode();
void displaySetHostPresent(bool present);
void displayRefresh();
void displaySetStats(int cpu, int ram, int cpuTemp, int gpu, int gpuTemp, int vram);
void displaySetLayer(uint8_t index, const String &name, const String labels[NUM_BUTTONS]);
void displayOtaProgress(int pct);
void displayOtaEnd(bool ok);
