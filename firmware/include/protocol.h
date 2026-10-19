#pragma once

#include <Arduino.h>

void protocolSetup();
void protocolLoop();  // lee lineas entrantes y despacha al modulo de display

void protocolSendHello();
void protocolSendButton(uint8_t id, bool pressed);
void protocolSendEncoder(uint8_t id, int8_t delta);
