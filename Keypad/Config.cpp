/*
 * This module manages configuration data on local flash storage
 *
 * Copyright (c) 2017, 2018 Danny Backx
 *
 * License (GNU Lesser General Public License) :
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 3 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#undef	USE_SPIFFS

#include <Arduino.h>
#ifdef ESP8266
# include <ESP8266WiFi.h>
#else
# include <WiFi.h>
#endif
#include <Config.h>
#include <secrets.h>
#ifdef	USE_SPIFFS
# include <FS.h>
# if defined(ESP32)
#  include <SPIFFS.h>
# endif
#endif
#include <ArduinoJson.h>
#include <preferences.h>

Config::Config() {
  name = 0;
#ifdef	USE_SPIFFS
  SPIFFS.begin();
#endif

  radio_pin = -1;
  siren_pin = -1;
  oled = false;
  rfid = false;
  secure = false;

  String mac = WiFi.macAddress();
  HardCodedConfig(mac.c_str());
  ReadConfig();
}

Config::~Config() {
#ifdef	USE_SPIFFS
  SPIFFS.end();
#endif
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
#ifdef	USE_SPIFFS
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
#endif
}

void Config::ReadConfig(const char *js) {
  Serial.printf("ReadConfig %s\n", js);

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
  radio_pin = jo["radioPin"] | -1;
  oled = jo["haveOled"] | false;
  oled_led_pin = jo["oledLedPin"] | -1;
  i2c_sda_pin = jo["i2cSdaPin"] | -1;
  i2c_scl_pin = jo["i2cSclPin"] | -1;

  name = jo["name"];
  // Note the missing else case gets treated in Config::myName()
  if (name) {
    name = strdup(name);	// Storage from JSON library doesn't last
  }

  const char *rfidType = jo["rfidType"];
  rfid = (rfidType != 0);
  rfid_rst_pin = jo["rfidRstPin"] | -1;
  rfid_ss_pin = jo["rfidSsPin"] | -1;
  if (rfid_rst_pin < 0 || rfid_ss_pin < 0)
    rfid = false;

  weather = jo["weather"];
  secure = jo["secure"];
}

void Config::HardCodedConfig(const char *mac) {
  for (int i=0; configs[i].mac != 0; i++)
    if (strcasecmp(configs[i].mac, mac) == 0) {
      Serial.printf("Hardcoded config %s\n", mac);
      ReadConfig(configs[i].config);
      return;
    }
  Serial.printf("No hardcoded config for %s\n", mac);
}

void Config::WriteConfig() {
#ifdef	USE_SPIFFS
  SPIFFS.remove(PREF_CONFIG_FN);

  File f = SPIFFS.open(PREF_CONFIG_FN, "w");
  if (!f) {
    Serial.printf("Failed to save config to %s\n", PREF_CONFIG_FN);
    return;
  }
  DynamicJsonBuffer jb;
  JsonObject &json = jb.createObject();
  char	siren_pin_s[8],
	radio_pin_s[8],
	led_pin_s[8];

  sprintf(siren_pin_s, "%d", siren_pin);
  sprintf(radio_pin_s, "%d", radio_pin);
  sprintf(led_pin_s, "%d", oled_led_pin);
  json["sirenPin"] = siren_pin_s;
  json["radioPin"] = radio_pin_s;
  json["oledLedPin"] = led_pin_s;

  json["rfidType"] = "mfrc522";
  json["haveRfid"] = rfid;
  json["rfidSsPin"] = rfid_ss_pin;
  json["rfidRstPin"] = rfid_rst_pin;
  json["weather"] = weather;
  json["secure"] = secure;

  if (json.printTo(f) == 0) {
    Serial.printf("Failed to write to config file %s\n", PREF_CONFIG_FN);
    return;
  }
  f.close();
#endif
}

boolean Config::haveOled() {
  return oled;
}

int Config::GetOledLedPin() {
  return oled_led_pin;
}

boolean Config::haveRadio() {
  return (radio_pin >= 0);
}

boolean Config::haveRfid() {
  return rfid;
}

boolean Config::haveWeather() {
  return weather;
}

boolean Config::haveSecure() {
  return secure;
}

/*
 * Hardcoded configuration JSON per MAC address
 * Store these in secrets.h in the MODULES_CONFIG_STRING macro definition.
 */
struct config Config::configs[] = {
#if 0
  { "12:34:56:78:90:ab",
    "{ \"radioPin\" : 4, \"haveOled\" : true, \"name\" : \"Keypad gang\" }"
  },
  { "01:23:45:67:89:0a",
    "{ \"name\" : \"ESP32 d1 mini\" }"
  },
#endif
  MODULES_CONFIG_STRING
  { 0, 0 }
};

const char *Config::myName(void) {
  if (name == 0) {
    name = (char *)malloc(40);
    String mac = WiFi.macAddress();
    sprintf((char *)name, "Controller %s", mac.c_str());
  }
  return name;
}

boolean Config::DSTEurope() {
  return true;
}

boolean Config::DSTUSA() {
  return false;
}

int Config::GetI2cSdaPin() {
  return i2c_sda_pin;
}

int Config::GetI2cSclPin() {
  return i2c_scl_pin;
}

