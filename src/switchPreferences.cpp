#include "headers/constants.h"
#include "headers/switchPreferences.h"

Preferences preferences;

void setConfig(bool val)
{
  preferences.begin(ns, false);
  preferences.putBool(CONFIG_KEY, val);
}

bool hasConfig()
{
  preferences.begin(ns, false);
  bool hasConfig = preferences.getBool(CONFIG_KEY);
  preferences.end();
  return hasConfig;
}

void addPreferences(Pref prefs[])
{
  Serial.println("Size of pref array");
  Serial.println(sizeof(prefs));

  preferences.begin(ns, false);
  for (int i = 0; i < 4; i++)
  {
    Pref p = prefs[i];
    String n = p.name;
    String v = p.value;
    preferences.putString(p.name.c_str(), p.value.c_str());
  }
  preferences.end();
}
