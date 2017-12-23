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

Config::Config() {
  SPIFFS.begin();
  File f = SPIFFS.open("config.json", "r");
  if (!f) {
    Serial.println("Config: can not open config.xml");
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

Config::~Config() {
}

int Config::GetRadioPin() {
  return A0;
}

void Config::SetRadioPin(int pin) {
}

int Config::GetSirenPin() {
  return -1;
}

void Config::SetSirenPin(int pin) {
}

void Config::ReadConfig() {
}

void Config::WriteConfig() {
  SPIFFS.remove("config.json");

  File f = SPIFFS.open("config.json", "w");
  if (!f) {
    Serial.println("Failed to save config");
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
    Serial.println("Failed to write to config file");
    return;
  }
  f.close();
}
