#pragma once

#include <Arduino.h>

// Callbacks que el main.cpp registra para reaccionar a eventos de input.
// id: índice 0-based dentro de BUTTON_PINS / ENCODER_PINS.
using ButtonCallback = void (*)(uint8_t id, bool pressed);
using EncoderCallback = void (*)(uint8_t id, int8_t delta);

void inputsSetup();
void inputsLoop();
void inputsOnButton(ButtonCallback cb);
void inputsOnEncoder(EncoderCallback cb);
