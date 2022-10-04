#ifndef _ESP_H
#define _ESP_H
#pragma once

#include "Arduino.h"

void rebootESP(String message);
void deleteFile(String filename);
void flash(String filename);
void resetConfig();
void magnetResetConfig();
void initSpiffs();
String listFiles(bool ishtml);
String humanReadableSize(const size_t bytes);

#endif