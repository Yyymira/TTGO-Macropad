#pragma once

#include <Arduino.h>

#include "config.h"

enum class DisplayMode : uint8_t { Idle, Layer };

void displaySetup();
void displayLoop();  // alternancia stats/clima, reloj, indicador BT
void displaySetMode(DisplayMode mode);
DisplayMode displayGetMode();
void displaySetHostPresent(bool present);
// Fija la pantalla de reposo: 0 = stats, 1 = clima, 2 = multimedia, -1 = la configurada.
void displaySetViewOverride(int8_t view);
void displayRefresh();
void displaySetStats(int cpu, int ram, int cpuTemp, int gpu, int gpuTemp, int vram);
void displaySetLayer(uint8_t index, const String &name, const String labels[NUM_BUTTONS]);
void displaySetMeeting(bool active, bool mic);  // reunión en el navegador: indicador de micrófono
void displaySetLayerToPhone(bool toPhone);  // la capa activa manda sus teclas al celular (antes de displaySetLayer)
void displayOtaProgress(int pct);
void displayOtaEnd(bool ok);
