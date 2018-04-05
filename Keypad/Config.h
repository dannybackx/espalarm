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

#ifndef	_CONFIG_H_
#define	_CONFIG_H_

#include <ArduinoJson.h>
#ifdef DEPRECATED
// Avoid duplicate definition from ArduinoJson.h and MFRC522.h
#undef DEPRECATED
#endif

struct config {
  const char *mac;
  const char *config;
};

class Config {
public:
  Config();
  ~Config();

  int GetRadioPin();
  int GetSirenPin();

  void SetRadioPin(int);
  void SetSirenPin(int);

  boolean haveOled();
  const char *myName();
  boolean haveRfid();
  boolean haveRadio();
  int GetOledLedPin();
  boolean haveWeather();

  boolean haveSecure();

  boolean DSTEurope();
  boolean DSTUSA();

  int GetI2cSdaPin();
  int GetI2cSclPin();

private:
  int siren_pin;
  int radio_pin;

  boolean oled;
  const char *name;
  int oled_led_pin;

  boolean rfid;
  boolean weather;
  boolean secure;

  const char *rfidType;	// mfrc522 or pn532
  int rfid_rst_pin, rfid_ss_pin;

  // i2c
  int i2c_sda_pin, i2c_scl_pin;

  int dirty;
  void ReadConfig();
  void ReadConfig(const char *);
  void WriteConfig();
  void ParseConfig(JsonObject &jo);
  void HardCodedConfig(const char *mac);

  static struct config configs[];
};

extern Config *config;

#endif	/* _CONFIG_H_ */
