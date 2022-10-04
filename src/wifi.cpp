#include <WiFi.h>
#include "headers/constants.h"
#include "headers/esp.h"
#include "headers/led.h"
#include "headers/wifi.h"
#include "headers/switchPreferences.h"

int GetWifiStatus()
{
  return (int)WiFi.status();
}

void setupWifi(WifiCredentials credentials)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("Name");
  WiFi.hostname(credentials.Name.c_str());
  Serial.println("SSID");
  Serial.println(credentials.Ssid.c_str());
  Serial.println("Password");
  Serial.println(credentials.Pwd.c_str());
  WiFi.begin(credentials.Ssid.c_str(), credentials.Pwd.c_str());

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("."); // Affiche des points .... tant que connexion n'est pas OK
    delay(100);
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;

    if (elapsedTime > PERIOD)
    {
      resetConfig();
    };
  }

  Serial.println("\n");
  Serial.println("Connexion etablie !"); // Affiche connexion établie
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP()); // Affiche l'adresse IP de l'ESP32 avec WiFi.localIP

  for (int i = 0; i < 4; i++)
  {
    flashLed();
    delay(500);
  }
}

WifiCredentials getWifiCredentials()
{
  preferences.begin(ns, false);
  String ssid = preferences.getString(SSID_PARAM.c_str());
  String pwd = preferences.getString(PASSWORD_PARAM.c_str());
  String name = preferences.getString(NAME_PARAM.c_str());
  WifiCredentials credentials = WifiCredentials(ssid, pwd, name);
  preferences.end();
  return credentials;
}