#ifndef CONSTANTS_H
#define CONSTANTS_H
#pragma once

#include "Arduino.h"

const char *ns = "switch-light";
const char *deviceName = "switch1";

const char *DEFAULT_SSID = "Nico's Mind Smartswitch AP";
const char *DEFAULT_PASSWORD = "123456789";
const char *CONFIG_KEY = "ConfigWifi";
const unsigned long PERIOD = 10000;
const String SSID_PARAM = "ssid";
const String PASSWORD_PARAM = "password";
const String NAME_PARAM = "name";
const String LOCATION_PARAM = "location";
const String DEFAULT_NAME = "NicosMind Switch";

#define LED_PIN 32
#define NUM_LEDS 8
CRGB leds[NUM_LEDS];
uint8_t max_bright = 150;

#endif