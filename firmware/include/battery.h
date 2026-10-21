#pragma once

#include <Arduino.h>

// Batería LiPo de la T-Display: se lee por GPIO34 a través de un divisor 1/2
// que la placa habilita con GPIO14 (ADC_EN). Con USB enchufado el voltaje
// no dice cuánta carga queda (el cargador lo sube), así que ahí solo se
// informa "enchufada".
void batterySetup();
void batteryLoop();

bool batteryOnUsb();
int batteryPercent();       // 0-100, -1 hasta la primera lectura
bool batteryLow();          // a batería y <= BATTERY_LOW_PCT
bool batteryCritical();     // a batería y <= BATTERY_CRITICAL_PCT
bool batteryConsumeChange();  // true si cambió algo que la pantalla muestra
