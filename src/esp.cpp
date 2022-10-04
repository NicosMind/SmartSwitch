#include "Arduino.h"
#include "SPIFFS.h"
#include "Update.h"
#include "switchPreferences.h"
#include "driver/adc.h"
#include "led.h"
#include "constants.h"

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