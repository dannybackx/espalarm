/*
 * This module manages wireless sensors, e.g. Kerui PIR motion detectors or smoke detectors.
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

#ifndef	_WIRELESS_SENSOR_H_
#define	_WIRELESS_SENSOR_H_

#include "RCSwitch.h"

enum SensorStatus {
  SENSOR_
};

class Sensors {
public:
  Sensors();
  ~Sensors();
  void loop(time_t);
  void AddSensor(int id, const char *name);

  // void SetStatus(BackLightStatus);
  // void Trigger(time_t);
  // void SetTimeout(int);

private:
  // time_t trigger_ts;
  enum SensorStatus	status;
  int			id;

  RCSwitch		radio;

  // int led_pin;
  // int timeout;		// number of seconds for backlight to stay lit
};

#endif	/* _WIRELESS_SENSOR_H_ */
