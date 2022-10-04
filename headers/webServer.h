#ifndef WEBSERVER_H
#define WEBSERVER_H
#pragma once

#include "Arduino.h"
#include <ESPAsyncWebServer.h>
#include "Preferences.h"
#include "headers/switchPreferences.h"

extern AsyncWebServer server(80);


void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void initWebServer();
void initWebServerConfig();

#endif