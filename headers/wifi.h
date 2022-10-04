#ifndef _WIFI_H
#define _WIFI_H
#pragma once

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

int GetWifiStatus();
void setupWifi(WifiCredentials credentials);

WifiCredentials getWifiCredentials();

#endif
