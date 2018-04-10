/*
 * This module manages Alarm state and signaling
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

#include <Oled.h>

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
  ZONE_HID,		// Human interface : an rfid card, someone entered his pin, ...
};

class Alarm {
public:
  Alarm(Oled *oled);
  ~Alarm();
  void loop(time_t);

  void SetArmed(AlarmStatus);
  void SetArmed(const char *);
  void SetArmed(AlarmStatus s, AlarmZone zone);
  AlarmStatus GetArmed();
  const char *GetArmedString();

  void Signal(const char *sensor, AlarmZone zone);	// Still to decide based on zone
  void SoundAlarm(const char *sensor);			// We've decided : just start yelling
  void Reset(const char *module);			// From a peer controller
  void Reset(time_t nowts, const char *user);		// Local
  void Toggle(time_t nowts, const char *user);		// Local

private:
  enum AlarmStatus	armed;	// Armed or not
  boolean		alert;
  Oled			*oled;

  OledButton		*alarmButton;
  void			AlarmButtonPressed();
  void			AlarmButtonUnpressed();
  uint32_t		lasttime;
};

extern Alarm *_alarm;
#endif	/* _ALARM_STATE_H_ */
