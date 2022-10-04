#include "Arduino.h"
#include <ESPAsyncWebServer.h>
#include "headers/constants.h"
#include "Preferences.h"
#include <SPIFFS.h>
#include "headers/webServer.h"
#include "switchPreferences.h"
#include "headers/esp.h"

AsyncWebServer server(80);

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  Serial.println(logmessage);

  if (!index) {
    logmessage = "Upload Start: " + String(filename);
    // open the file on first call and store the file handle in the request object
    request->_tempFile = SPIFFS.open("/" + filename, "w");
    Serial.println(logmessage);
  }

  if (len) {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data, len);
    logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
    Serial.println(logmessage);
  }

  if (final) {
    logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
    // close the file handle as the upload is now done
    request->_tempFile.close();
    Serial.println(logmessage);

    flash(filename);
    //request->redirect("/");
  }
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

void initWebServerConfig(Preferences preferences)
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

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