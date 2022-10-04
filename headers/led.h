#ifndef LED_H
#define LED_H
#pragma once

#include "Arduino.h"

void setLedColor();
void flashLed();
void CircleColor(uint8_t R, uint8_t G, uint8_t B);
void initLeds();

#endif