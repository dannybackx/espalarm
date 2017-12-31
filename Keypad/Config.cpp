/*
 * This module manages configuration data on local flash storage
 *
 * Copyright (c) 2017 Danny Backx
 *
 * License (MIT license):
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <Config.h>
#include <secrets.h>
#include <FS.h>
#if defined(ESP32)
#include <SPIFFS.h>
#endif
#include <ArduinoJson.h>
#include <preferences.h>

Config::Config() {
  SPIFFS.begin();

#if defined(ESP32)
  radio_pin = 2;
#elif defined(ESP8266)
  radio_pin = D2;
#endif
  siren_pin = -1;
  oled = false;

  String mac = WiFi.macAddress();
  HardCodedConfig(mac.c_str());
  ReadConfig();
}

Config::~Config() {
  SPIFFS.end();
}

int Config::GetRadioPin() {
  return radio_pin;
}

void Config::SetRadioPin(int pin) {
  radio_pin = pin;
  WriteConfig();
}

int Config::GetSirenPin() {
  return siren_pin;
}

void Config::SetSirenPin(int pin) {
  siren_pin = pin;
  WriteConfig();
}

void Config::ReadConfig() {
  File f = SPIFFS.open(PREF_CONFIG_FN, "r");
  if (!f)
    return;	// Silently

  DynamicJsonBuffer jb;
  JsonObject &json = jb.parseObject(f);
  if (json.success()) {
    Serial.printf("Reading config from SPIFFS %s\n", PREF_CONFIG_FN);
    ParseConfig(json);
  } else {
    Serial.printf("Could not parse JSON from %s\n", PREF_CONFIG_FN);
  }

  f.close();
}

void Config::ReadConfig(const char *js) {
  DynamicJsonBuffer jb;
  JsonObject &json = jb.parseObject(js);
  if (json.success()) {
    ParseConfig(json);
  } else {
    Serial.println("Could not parse JSON");
  }
}

void Config::ParseConfig(JsonObject &jo) {
  siren_pin = jo["sirenPin"] | -1;
  radio_pin = jo["radioPin"] | A0;
  oled = jo["haveOled"] | false;
}

void Config::HardCodedConfig(const char *mac) {
  for (int i=0; configs[i].mac != 0; i++)
    if (strcasecmp(configs[i].mac, mac) == 0) {
      Serial.printf("Hardcoded config %s\n", mac);
      ReadConfig(configs[i].config);
      return;
    }
}

void Config::WriteConfig() {
  SPIFFS.remove(PREF_CONFIG_FN);

  File f = SPIFFS.open(PREF_CONFIG_FN, "w");
  if (!f) {
    Serial.printf("Failed to save config to %s\n", PREF_CONFIG_FN);
    return;
  }
  DynamicJsonBuffer jb;
  JsonObject &json = jb.createObject();
  char	siren_pin_s[8],
	radio_pin_s[8];
  sprintf(siren_pin_s, "%d", siren_pin);
  sprintf(radio_pin_s, "%d", radio_pin);
  json["sirenPin"] = siren_pin_s;
  json["radioPin"] = radio_pin_s;

  if (json.printTo(f) == 0) {
    Serial.printf("Failed to write to config file %s\n", PREF_CONFIG_FN);
    return;
  }
  f.close();
}

boolean Config::haveOled() {
  return oled;
}

/*
 * Hardcoded configuration JSON per MAC address
 * Store these in secrets.h in the MODULES_CONFIG_STRING macro definition.
 */
struct config Config::configs[] = {
#if 0
  { "12:34:56:78:90:ab",
    "{ \"radioPin\" : 2, \"haveOled\" : true, \"name\" : \"Keypad gang\" }"
  },
  { "01:23:45:67:89:0a",
    "{ \"name\" : \"ESP32 d1 mini\" }"
  },
#endif
  MODULES_CONFIG_STRING
  { 0, 0 }
};
