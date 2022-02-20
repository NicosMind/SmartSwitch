#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "driver/adc.h"

#define LED_PIN 32
#define NUM_LEDS 8
CRGB leds[NUM_LEDS];
uint8_t max_bright = 150;

Preferences preferences;

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

AsyncWebServer server(80);

class WifiCredentials
{
public:
  String Ssid;
  String Pwd;
  String Name;
  WifiCredentials(String ssid, String pwd, String name)
  {
    Ssid = ssid;
    Pwd = pwd;
    Name = name;
  }
};

void setLedColor()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}
void flashLed()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Blue;
  }
  FastLED.show();
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void CircleColor(uint8_t R, uint8_t G, uint8_t B)
{
  for (int i = 0; i <= NUM_LEDS; i++)
  {
    leds[i] = CRGB(R, G, B);
    FastLED.show();

    delay(50);
  }
}

void initLeds()
{
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(max_bright);
  set_max_power_in_volts_and_milliamps(5, 1000);
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

struct Pref
{
  String name;
  String value;
};

int GetWifiStatus()
{
  return (int)WiFi.status();
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

void setConfig(bool val)
{
  preferences.begin(ns, false);
  preferences.putBool(CONFIG_KEY, val);
}

void resetConfig()
{
  setConfig(false);
  ESP.restart();
}

void magnetResetConfig()
{
  if (hall_sensor_read() > 25)
  {
    for (int i = 0; i <= 5; i++)
    {
      CircleColor(128, 128, 0);
    }
    Serial.println("ENABLE CONFIG MODE");
    resetConfig();
  }
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

void initSpiffs()
{
  if (!SPIFFS.begin()) /* Démarrage du gestionnaire de fichiers SPIFFS */
  {
    Serial.println("Erreur SPIFFS...");
    return;
  }

  /* Détection des fichiers présents sur l'Esp32 */
  File root = SPIFFS.open("/");    /* Ouverture de la racine */
  File file = root.openNextFile(); /* Ouverture du 1er fichier */
  while (file)                     /* Boucle de test de présence des fichiers - Si plus de fichiers la boucle s'arrête*/

  {
    Serial.print("File: ");
    Serial.println(file.name());
    file.close();
    file = root.openNextFile(); /* Lecture du fichier suivant */
  }
}

void initWebServerConfig()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("get page");
    request->send(SPIFFS, "/index.html", "text/html"); });

  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/bootstrap.min.css", "text/css"); });

  server.on("/img.png", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/img.png", "image/png"); });

  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hello World"); });

  server.on("/Setup", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              String ssidValue;
              String passwordValue;
              String nameValue;
              String locationValue;

              if (request->hasParam(SSID_PARAM, true)) {
                ssidValue = request->getParam(SSID_PARAM, true)->value();
              }

              if (request->hasParam(PASSWORD_PARAM, true)) {
                passwordValue = request->getParam(PASSWORD_PARAM, true)->value();
              }

              if (request->hasParam(NAME_PARAM, true)) {
                nameValue = request->getParam(NAME_PARAM, true)->value();
              }else {
                nameValue = DEFAULT_NAME;
              }

              if (request->hasParam(LOCATION_PARAM, true)) {
                locationValue = request->getParam(LOCATION_PARAM, true)->value();
              }

              if(ssidValue != NULL && passwordValue != NULL && locationValue != NULL) {
                Pref ssidPreference, passwordPreference, namePreference, locationPreference;
                ssidPreference.name = SSID_PARAM;
                ssidPreference.value = ssidValue;

                passwordPreference.name = PASSWORD_PARAM;
                passwordPreference.value = passwordValue;

                namePreference.name = NAME_PARAM;
                namePreference.value = nameValue;

                locationPreference.name = LOCATION_PARAM;
                locationPreference.value = locationValue;
                
                Pref prefs[] = {ssidPreference, passwordPreference, namePreference, locationPreference};

                addPreferences(prefs);
                setConfig(true);
                request->send(200, "text/plain", "saved ok");
                
                ESP.restart();
              }else {
                request->send(401);
              } });

  /* On affiche que le serveur est actif */
  server.begin();
}

void initWebServer()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("get page");
    request->send(SPIFFS, "/firmware.html", "text/html"); });

  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/bootstrap.min.css", "text/css"); });

  server.on("/img.png", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/img.png", "image/png"); });

  /* On affiche que le serveur est actif */
  server.begin();
}

bool hasConfig()
{
  preferences.begin(ns, false);
  bool hasConfig = preferences.getBool(CONFIG_KEY);
  preferences.end();
  return hasConfig;
}

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
  }else {
    CircleColor(0, 0, 0);
  }
  

  magnetResetConfig();

  // touchPadRoutine();
  //  Serial.println(hall_sensor_read());
  //   WiFi.scanNetworks will return the number of networks found
  //   int n = WiFi.scanNetworks();
  //   Serial.println("scan done");
  //   if (n == 0)
  //   {
  //     Serial.println("no networks found");
  //   }
  //   else
  //   {
  //     Serial.print(n);
  //     Serial.println(" networks found");
  //     for (int i = 0; i < n; ++i)
  //     {
  //       // Print SSID and RSSI for each network found
  //       Serial.print(i + 1);
  //       Serial.print(": ");
  //       Serial.print(WiFi.SSID(i));
  //       Serial.print(" (");
  //       Serial.print(WiFi.RSSI(i));
  //       Serial.print(")");
  //       Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
  //       delay(10);
  //     }
  //   }
  //   Serial.println("");
}