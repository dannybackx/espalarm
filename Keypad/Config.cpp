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
#include <Config.h>
#include <secrets.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <preferences.h>

Config::Config() {
#ifdef ESP32
  FS.begin();
#else
  SPIFFS.begin();
#endif

  radio_pin = D2;
  siren_pin = -1;

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
  if (!f) {
    Serial.printf("Config: can not open %s\n", PREF_CONFIG_FN);
#if 0
    FSInfo fsi;
    SPIFFS.info(fsi);
    Serial.printf("SPIFFS total %d (%d M) used %d (%d M)\n",
      fsi.totalBytes, fsi.totalBytes / 1024 / 1024,
      fsi.usedBytes, fsi.usedBytes / 1024 / 1024);
#endif
    return;
  }

  DynamicJsonBuffer jb;
  JsonObject &json = jb.parseObject(f);
  if (! json.success()) {
    Serial.println("Could not parse JSON");
    f.close();
    return;
  }

  siren_pin = json["sirenPin"] | -1;
  radio_pin = json["radioPin"] | A0;

  f.close();
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
