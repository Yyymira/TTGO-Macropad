#pragma once

#include <Arduino.h>

// Modo autónomo (sin PC): la propia placa maneja capas y timeout, y manda
// las teclas por Bluetooth usando el mapeo que la PC le copió.
void standaloneBegin();  // carga el mapeo guardado en la flash
void standaloneSetKeymap(const String &layersJson);
void standaloneActivate(bool on);
void standaloneButton(uint8_t id);
void standaloneEncoder(uint8_t id, int8_t delta);
void standaloneLoop();
