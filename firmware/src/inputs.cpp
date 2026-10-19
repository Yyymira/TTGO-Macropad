#include "inputs.h"
#include "config.h"

#include <Bounce2.h>
#include <ESP32Encoder.h>

static Bounce2::Button buttons[NUM_BUTTONS];
static ESP32Encoder encoders[NUM_ENCODERS];
static int32_t lastEncoderCount[NUM_ENCODERS] = {0};

static ButtonCallback buttonCb = nullptr;
static EncoderCallback encoderCb = nullptr;

void inputsOnButton(ButtonCallback cb) { buttonCb = cb; }
void inputsOnEncoder(EncoderCallback cb) { encoderCb = cb; }

void inputsSetup() {
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        // GPIO35 no tiene pull-up interno (pin input-only), pero la placa
        // ya trae un pull-up externo para el botón integrado, así que
        // configurar INPUT_PULLUP acá es inofensivo y uniforme para todos.
        buttons[i].attach(BUTTON_PINS[i], INPUT_PULLUP);
        buttons[i].setPressedState(LOW);
        buttons[i].interval(BUTTON_DEBOUNCE_MS);
    }

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        encoders[i].attachHalfQuad(ENCODER_PINS[i].clk, ENCODER_PINS[i].dt);
        encoders[i].setCount(0);
        lastEncoderCount[i] = 0;
    }
}

void inputsLoop() {
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].update();
        if (buttons[i].pressed() && buttonCb) buttonCb(i, true);
        if (buttons[i].released() && buttonCb) buttonCb(i, false);
    }

    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        int32_t count = encoders[i].getCount();
        int32_t diff = count - lastEncoderCount[i];
        if (diff != 0 && encoderCb) {
            // attachHalfQuad cuenta 2 pulsos por detent; normalizamos a ±1 por paso.
            int8_t steps = diff / 2;
            if (steps != 0) {
                encoderCb(i, steps > 0 ? 1 : -1);
                lastEncoderCount[i] += steps * 2;
            }
        }
    }
}
