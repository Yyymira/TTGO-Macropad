#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "inputs.h"
#include "protocol.h"

static void onButton(uint8_t id, bool pressed) { protocolSendButton(id, pressed); }
static void onEncoder(uint8_t id, int8_t delta) { protocolSendEncoder(id, delta); }

void setup() {
    Serial.begin(SERIAL_BAUD);
    displaySetup();
    inputsSetup();
    inputsOnButton(onButton);
    inputsOnEncoder(onEncoder);
    protocolSetup();
    protocolSendHello();
}

void loop() {
    inputsLoop();
    protocolLoop();
}
