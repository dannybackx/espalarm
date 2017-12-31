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

#ifndef	_CONFIG_H_
#define	_CONFIG_H_

#include <ArduinoJson.h>

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

private:
  int siren_pin;
  int radio_pin;

  boolean oled;
  const char *name;

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
