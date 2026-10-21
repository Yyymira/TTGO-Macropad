#pragma once

#include <Arduino.h>

// --- Pines: ver docs/wiring.md para el detalle y el por qué de cada elección ---

constexpr uint8_t NUM_BUTTONS = 10;
constexpr uint8_t NUM_ENCODERS = 2;

// Botones 0-5: Cherry MX en grilla 3x2 (fila de atrás 0-1-2, de adelante
// 3-4-5), cada uno entre su GPIO y GND con INPUT_PULLUP.
// Botones 6 y 7: los integrados en la placa (GPIO0 / GPIO35).
// Botones 8 y 9: el pulsador del eje de los encoders 1 y 2 (GPIO15 / GPIO2).
// Son pines de "strapping", pero el pulsador solo los lleva a GND al
// apretarlo, así que sueltos no cambian el arranque.
constexpr int8_t BUTTON_PINS[NUM_BUTTONS] = {32, 33, 25, 26, 13, 17, 0, 35, 15, 2};

struct EncoderPins {
    int8_t clk;
    int8_t dt;
};

constexpr EncoderPins ENCODER_PINS[NUM_ENCODERS] = {
    {27, 12},  // Encoder 0 (GPIO14 no sale a los pines de la T-Display: la placa lo usa
               // para habilitar la lectura de batería)
    {21, 22},  // Encoder 1
};

constexpr unsigned long BUTTON_DEBOUNCE_MS = 12;
constexpr uint32_t SERIAL_BAUD = 115200;

// Protocolo: una línea JSON por mensaje, en ambos sentidos. El mensaje más
// grande es "keymap" (todas las capas), por eso el margen.
constexpr size_t JSON_LINE_MAX = 4096;
constexpr size_t SERIAL_RX_BUFFER = 4096;

// Si la PC no manda nada en este tiempo, la placa pasa a modo autónomo:
// reloj/clima en pantalla y teclas por Bluetooth.
constexpr uint32_t HOST_TIMEOUT_MS = 5000;

constexpr uint32_t WEATHER_PERIOD_MS = 15UL * 60 * 1000;
constexpr uint32_t WEATHER_RETRY_MS = 60UL * 1000;
constexpr uint32_t IDLE_ALTERNATE_MS = 10UL * 1000;  // modo "ambos": stats <-> clima
constexpr uint32_t NET_REPORT_MS = 5000;
constexpr const char *DEVICE_HOSTNAME = "ttgo-macropad";

// Batería (ver battery.h)
constexpr uint8_t BATTERY_ADC_PIN = 34;
constexpr uint8_t BATTERY_ADC_EN_PIN = 14;
constexpr uint32_t BATTERY_SAMPLE_MS = 2000;
constexpr float BATTERY_USB_VOLTS = 4.30;   // por encima: alimentada por USB (o cargando)
constexpr int BATTERY_LOW_PCT = 15;         // aviso en pantalla
constexpr int BATTERY_CRITICAL_PCT = 5;     // baja el brillo y apaga el WiFi
constexpr uint8_t BACKLIGHT_DIM = 40;       // brillo en modo crítico (0-255)
constexpr uint8_t BACKLIGHT_CHANNEL = 7;    // canal LEDC del backlight
