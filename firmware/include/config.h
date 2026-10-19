#pragma once

#include <Arduino.h>

// --- Pines: ver docs/wiring.md para el detalle y el por qué de cada elección ---

constexpr uint8_t NUM_BUTTONS = 6;
constexpr uint8_t NUM_ENCODERS = 2;

// Botones 0 y 1 son los integrados en la placa (GPIO0 / GPIO35).
// Botones 2-5 se cablean a protoboard; todos a GND con INPUT_PULLUP.
constexpr int8_t BUTTON_PINS[NUM_BUTTONS] = {0, 35, 32, 33, 25, 26};

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
