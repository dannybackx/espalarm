/*
 * This module controls the TFT backlight
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

#ifndef	_BACKLIGHT_H_
#define	_BACKLIGHT_H_

#include "TimeLib.h"

enum BackLightStatus {
  BACKLIGHT_NONE,	// Out, not initialized
  BACKLIGHT_ON,		// Permanently on
  BACKLIGHT_TEMP_OFF,
  BACKLIGHT_TEMP_ON,
};

class BackLight {
public:
  BackLight(int pin);
  ~BackLight();
  void loop(time_t);
  void SetStatus(BackLightStatus);
  void Trigger(time_t);
  void SetTimeout(int);
  void SetBrightness(int percentage);

private:
  time_t trigger_ts;
  enum BackLightStatus	status;

  int led_pin;
  int brightness;
  int timeout;		// number of seconds for backlight to stay lit
};

#endif	/* _BACKLIGHT_H_ */
