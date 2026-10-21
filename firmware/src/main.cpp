#include <Arduino.h>

#include "battery.h"
#include "ble_kbd.h"
#include "config.h"
#include "display.h"
#include "inputs.h"
#include "net.h"
#include "protocol.h"
#include "settings.h"
#include "standalone.h"

// Con la PC conectada (el servicio manda datos), los eventos van por serial
// y la PC decide qué hacer. Sin PC, la placa usa su copia del mapeo y manda
// las teclas por Bluetooth.
static bool hostPresent = false;

static void onButton(uint8_t id, bool pressed) {
    if (hostPresent) {
        protocolSendButton(id, pressed);
    } else if (pressed) {
        standaloneButton(id);
    }
}

static void onEncoder(uint8_t id, int8_t delta) {
    if (hostPresent) {
        protocolSendEncoder(id, delta);
    } else {
        standaloneEncoder(id, delta);
    }
}

void setup() {
    Serial.setRxBufferSize(SERIAL_RX_BUFFER);
    Serial.begin(SERIAL_BAUD);
    settingsLoad();
    batterySetup();
    displaySetup();
    inputsSetup();
    inputsOnButton(onButton);
    inputsOnEncoder(onEncoder);
    protocolSetup();
    standaloneBegin();
    bleSetup();
    netSetup();
    protocolSendHello();
}

void loop() {
    inputsLoop();
    protocolLoop();

    bool present = protocolHostAlive(HOST_TIMEOUT_MS);
    if (present != hostPresent) {
        hostPresent = present;
        standaloneActivate(!present);
        displaySetHostPresent(present);
    }

    static uint32_t lastNetReport = 0;
    if (hostPresent && millis() - lastNetReport >= NET_REPORT_MS) {
        lastNetReport = millis();
        protocolSendNet();
    }

    batteryLoop();
    netSetEnabled(!batteryCritical());
    standaloneLoop();
    netLoop();
    displayLoop();
}
