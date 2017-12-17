/*
 * This module controls the TFT backlight
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

private:
  time_t trigger_ts;
  enum BackLightStatus	status;

  int led_pin;
  int timeout;		// number of seconds for backlight to stay lit
};

#endif	/* _BACKLIGHT_H_ */
