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
  ALARM_OFF,		// Only ZONE_ALWAYS triggers the alarm
  ALARM_ON,		// Any sensor triggers the alarm
  ALARM_NIGHT,		// ZONE_SECURE sensors won't trigger alarm
  // ??
};

// A characteristic of both sensors and controllers
enum AlarmZone {
  ZONE_NONE,
  ZONE_SECURE,		// Sensors that you'll walk by at night
  			// A controller not in the perimeter (no need to authenticate)
  ZONE_PERIMETER,	// Most sensors are here
  			// Controllers require authentication before accepting actions
  ZONE_ALWAYS,		// Sensors that always trigger alarm (e.g. fire)
  ZONE_FROMPEER,	// Already evaluated, this is a real alarm passed from a peer controller
};

class Alarm {
public:
  Alarm();
  ~Alarm();
  void loop(time_t);

  void SetArmed(AlarmStatus);
  void SetArmed(AlarmStatus s, AlarmZone zone);

  void Signal(const char *sensor, AlarmZone zone);	// Still to decide based on zone
  void SoundAlarm(const char *sensor);			// We've decided : just start yelling
  void Reset(const char *module);			// From a peer controller
  void Reset(time_t nowts, const char *user);		// Local

private:
  enum AlarmStatus	armed;	// Armed or not
  boolean alert;
};

extern Alarm *_alarm;
#endif	/* _ALARM_STATE_H_ */
