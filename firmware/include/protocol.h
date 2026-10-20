#pragma once

#include <Arduino.h>

void protocolSetup();
void protocolLoop();  // lee lineas entrantes y despacha a display/ajustes/keymap

// true si la PC mandó algo en los últimos `timeoutMs`.
bool protocolHostAlive(uint32_t timeoutMs);

void protocolSendHello();
void protocolSendButton(uint8_t id, bool pressed);
void protocolSendEncoder(uint8_t id, int8_t delta);
void protocolSendNet();  // estado de WiFi/Bluetooth/clima, para mostrarlo en la PC
