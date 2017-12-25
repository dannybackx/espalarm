/*
 * This module manages Alarm state and signaling
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

#ifndef	_ALARM_STATE_H_
#define	_ALARM_STATE_H_

enum AlarmStatus {
  ALARM_OFF,
  ALARM_ON,
  // ??
};

enum AlarmZone {
  ZONE_NONE,
  ZONE_SECURE,
  ZONE_PERIMETER,
  ZONE_ALWAYS,
  ZONE_FROMPEER,	// Already evaluated, this is a real alarm passed from a peer controller
};

class Alarm {
public:
  Alarm();
  ~Alarm();
  void SetState(AlarmStatus);
  void loop(time_t);
  void Signal(const char *sensor, AlarmZone zone);	// Still to decide based on zone
  void SoundAlarm(const char *sensor);			// We've decided : just start yelling

private:
  // time_t trigger_ts;
  enum AlarmStatus	state;
  int			id;

  // RCSwitch		radio;

  // int led_pin;
  // int timeout;		// number of seconds for backlight to stay lit
};

extern Alarm *alarm;
#endif	/* _ALARM_STATE_H_ */
