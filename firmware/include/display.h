#pragma once

#include <Arduino.h>

#include "config.h"

enum class DisplayMode : uint8_t { Idle, Layer };

void displaySetup();
void displaySetMode(DisplayMode mode);
void displaySetStats(int cpu, int ram, int cpuTemp, int gpu, int gpuTemp, int vram);
void displaySetLayer(uint8_t index, const String &name, const String labels[NUM_BUTTONS]);
