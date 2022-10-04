#ifndef SWITCHPREFERENCES_H
#define SWITCHPREFERENCES_H
#pragma once

#include "Preferences.h"

extern Preferences preferences;

struct Pref
{
  String name;
  String value;
};

void setConfig(bool val);
bool hasConfig();
void addPreferences(Pref prefs[]);

#endif