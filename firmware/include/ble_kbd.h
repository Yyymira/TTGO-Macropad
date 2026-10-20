#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Teclado Bluetooth (BLE HID) para usar el macropad con el celular u otro
// dispositivo cuando la PC no está. Acepta los mismos nombres de tecla que
// la app de escritorio (ctrl, shift, t, playpause, volumeup, ...).
void bleSetup();
bool bleConnected();
void bleSendKeys(JsonArrayConst keys);
