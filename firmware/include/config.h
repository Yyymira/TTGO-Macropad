#pragma once

#include <Arduino.h>

// --- Pines: ver docs/wiring.md para el detalle y el por qué de cada elección ---

constexpr uint8_t NUM_BUTTONS = 8;
constexpr uint8_t NUM_ENCODERS = 2;

// Botones 0-5: Cherry MX en grilla 3x2 (fila de atrás 0-1-2, de adelante
// 3-4-5), cada uno entre su GPIO y GND con INPUT_PULLUP.
// Botones 6 y 7: los integrados en la placa (GPIO0 / GPIO35).
constexpr int8_t BUTTON_PINS[NUM_BUTTONS] = {32, 33, 25, 26, 13, 17, 0, 35};

struct EncoderPins {
    int8_t clk;
    int8_t dt;
};

constexpr EncoderPins ENCODER_PINS[NUM_ENCODERS] = {
    {27, 14},  // Encoder 0
    {21, 22},  // Encoder 1
};

constexpr unsigned long BUTTON_DEBOUNCE_MS = 12;
constexpr uint32_t SERIAL_BAUD = 115200;

// Protocolo: una línea JSON por mensaje, en ambos sentidos.
constexpr size_t JSON_LINE_MAX = 512;
