#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "Update.h"
#include "driver/adc.h"
#include "headers/constants.h"
#include "headers/switchPreferences.h"
#include "headers/wifi.h"
#include "headers/webServer.h"
#include "headers/led.h"
#include "headers/esp.h"

int a = 0;

void IRAM_ATTR setLeft()
{
  a = 1;
}

void IRAM_ATTR setRight()
{
  a = 0;
}

void IRAM_ATTR setMiddle()
{
}

void initInterrupts()
{
  attachInterrupt(25, setMiddle, RISING);
  attachInterrupt(26, setRight, RISING);
  attachInterrupt(27, setLeft, RISING);
}

void setup()
{
  Serial.begin(115200);
  pinMode(25, INPUT);
  pinMode(26, INPUT);
  pinMode(27, INPUT);

  initLeds();
  initInterrupts();

  if (hasConfig())
  {
    WifiCredentials credentials = getWifiCredentials();
    setupWifi(credentials);

    Serial.println("Mounting SPIFFS ...");
    if (!SPIFFS.begin(true))
    {
      // if you have not used SPIFFS before on a ESP32, it will show this error.
      // after a reboot SPIFFS will be configured and will happily work.
      Serial.println("ERROR: Cannot mount SPIFFS, Rebooting");
      rebootESP("ERROR: Cannot mount SPIFFS, Rebooting");
    }

    deleteFile("firmware.bin");
    deleteFile("toupload.txt");

    Serial.print("SPIFFS Free: ");
    Serial.println(humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes())));
    Serial.print("SPIFFS Used: ");
    Serial.println(humanReadableSize(SPIFFS.usedBytes()));
    Serial.print("SPIFFS Total: ");
    Serial.println(humanReadableSize(SPIFFS.totalBytes()));

    Serial.println(listFiles(false));

    Serial.print("Connecting...");
    Serial.print("IP address: ");
    Serial.print(WiFi.localIP());
    Serial.print(WiFi.getHostname());
    initSpiffs();
    initWebServer();
  }
  else
  {
    WiFi.softAP(DEFAULT_SSID, DEFAULT_PASSWORD);
    Serial.println();
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
    initSpiffs();
    initWebServerConfig();
  }
}

void loop()
{

  if (a == 1)
  {
    for (int i = 0; i <= 1; i++)
    {
      CircleColor(255, 0, 0);
    }
  }
  else
  {
    CircleColor(0, 0, 0);
  }

  Serial.println("Yo bitches");
  magnetResetConfig();

 
}