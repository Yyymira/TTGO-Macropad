#pragma once

#include <Arduino.h>

enum class DisplayMode : uint8_t { Idle, Layer };

void displaySetup();
void displaySetMode(DisplayMode mode);
void displaySetStats(int cpu, int ram, int cpuTemp, int gpu, int gpuTemp);
void displaySetLayer(uint8_t index, const String &name, const String labels[6]);
