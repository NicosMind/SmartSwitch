#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "Update.h"
#include "driver/adc.h"
#include <PubSubClient.h>

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

const char *mqtt_server = "10.0.0.74";
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

String listFiles(bool ishtml = false);

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
void rebootESP(String message)
{
  Serial.print("Rebooting ESP32: ");
  Serial.println(message);
  ESP.restart();
}

void deleteFile(String filename)
{
  String filePath = "/" + filename;
  if (SPIFFS.exists(filePath))
  {
    SPIFFS.remove(filePath);
  }
}

void flash(String filename)
{
  String filePath = "/" + filename;

  File file = SPIFFS.open(filePath);

  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("Starting update..");

  size_t fileSize = file.size();

  if (!Update.begin(fileSize))
  {

    Serial.println("Cannot do the update");
    return;
  };

  Update.writeStream(file);

  if (Update.end())
  {

    Serial.println("Successful update");
  }
  else
  {

    Serial.println("Error Occurred: " + String(Update.getError()));
    return;
  }

  file.close();

  Serial.println("Reset in 4 seconds...");

  delay(4000);

  rebootESP("Rebooting...");
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  Serial.println(logmessage);

  if (!index)
  {
    logmessage = "OTA Update Start: " + String(filename);
    Serial.println(logmessage);
    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
    { // start with max available size
      Update.printError(Serial);
    }
  }

  if (len)
  {
    // flashing firmware to ESP
    if (Update.write(data, len) != len)
    {
      Update.printError(Serial);
    }
    logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
    Serial.println(logmessage);
  }

  if (final)
  {
    if (Update.end(true))
    { // true to set the size to the current progress
      logmessage = "OTA Complete: " + String(filename) + ",size: " + String(index + len);
      Serial.println(logmessage);
    }
    else
    {
      Update.printError(Serial);
    }
    request->redirect("/");
    ESP.restart();
  }
}

String server_ui_size(const size_t bytes)
{
  if (bytes < 1024)
    return String(bytes) + " B";
  else if (bytes < (1024 * 1024))
    return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024))
    return String(bytes / 1024.0 / 1024.0) + " MB";
  else
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

String server_directory(bool ishtml)
{
  String returnText = "";
  Serial.println("Listing files stored on SPIFFS");
  File root = SPIFFS.open("/");
  File foundfile = root.openNextFile();
  if (ishtml)
  {
    returnText += "<table align='center'><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
  }
  while (foundfile)
  {
    if (ishtml)
    {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + server_ui_size(foundfile.size()) + "</td>";
      returnText += "<td><button class='directory_buttons' onclick=\"directory_button_handler(\'" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
      returnText += "<td><button class='directory_buttons' onclick=\"directory_button_handler(\'" + String(foundfile.name()) + "\', \'delete\')\">Delete</button></tr>";
    }
    else
    {
      returnText += "File: " + String(foundfile.name()) + " Size: " + server_ui_size(foundfile.size()) + "\n";
    }
    foundfile = root.openNextFile();
  }
  if (ishtml)
  {
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  return returnText;
}

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes)
{
  if (bytes < 1024)
    return String(bytes) + " B";
  else if (bytes < (1024 * 1024))
    return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024))
    return String(bytes / 1024.0 / 1024.0) + " MB";
  else
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

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

String listFiles(bool ishtml)
{
  String returnText = "";
  Serial.println("Listing files stored on SPIFFS");
  File root = SPIFFS.open("/");
  File foundfile = root.openNextFile();
  if (ishtml)
  {
    returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th></tr>";
  }
  while (foundfile)
  {
    if (ishtml)
    {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td></tr>";
    }
    else
    {
      returnText += "File: " + String(foundfile.name()) + "\n";
    }
    foundfile = root.openNextFile();
  }
  if (ishtml)
  {
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  return returnText;
}

/// config webserver bootstrap
void initWebServerConfig()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/bootstrap.min.css", "text/css"); });

  server.on("/img.png", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/img.png", "image/png"); });

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
    String logmessage = "Client:" + request->client()->remoteIP().toString() + + " " + request->url();
    Serial.println(logmessage);
    request->send(SPIFFS, "/firmware.html", "text/html"); });

  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/bootstrap.min.css", "text/css"); });

  server.on("/img.png", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/img.png", "image/png"); });

  server.on(
      "/UploadFile", HTTP_POST, [](AsyncWebServerRequest *request)
      { request->send(200); },
      handleUpload);

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

void mqttCallback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  // if (String(topic) == "esp32/output") {
  //   Serial.print("Changing output to ");
  //   if(messageTemp == "on"){
  //     Serial.println("on");
  //     digitalWrite(ledPin, HIGH);
  //   }
  //   else if(messageTemp == "off"){
  //     Serial.println("off");
  //     digitalWrite(ledPin, LOW);
  //   }
  // }
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

    Serial.println(listFiles());

    Serial.print("Connecting...");
    Serial.print("IP address: ");
    Serial.print(WiFi.localIP());
    Serial.print(WiFi.getHostname());
    initSpiffs();
    initWebServer();
    Serial.println(server_directory(false));

    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);
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

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // TODO change the name below and add to memory
    if (client.connect("SmartSwicthNameMustBeUnique"))
    {
      Serial.println("connected");
      // Subscribe
      client.subscribe("tanuki");
      client.setCallback(mqttCallback);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  if (a == 1)
  {
    client.publish("topic", "salut");
    for (int i = 0; i <= 1; i++)
    {
      CircleColor(125, 0, 0);
    }
  }
  else
  {
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