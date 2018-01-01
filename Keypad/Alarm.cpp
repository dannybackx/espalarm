/*
 * This module manages Alarm state and signaling
 *	Alarms need to be passed to peer controllers
 *	Alarms passed from peer controllers don't need to be forwarded
 *	Sirens (which may be flash lights as well as audible devices) need triggering.
 *	We could potentially connect/messsage to a smartphone app
 *	We could send e-mail
 *	User interface (the Oled modules) needs to show this as well
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

#include <Arduino.h>
#include <Alarm.h>
#include <Peers.h>
#include <secrets.h>

Alarm::Alarm() {
  armed = ALARM_OFF;
  alert = false;
}

Alarm::~Alarm() {
}

void Alarm::SetArmed(AlarmStatus s) {
  armed = s;
}

void Alarm::SetArmed(AlarmStatus s, AlarmZone zone) {
  armed = s;
  if (zone != ZONE_FROMPEER) {
    peers->AlarmSetArmed(s);
  }
}

/*
 * Still to decide based on zone
 * However, if ZONE_FROMPEER then don't forward to peers, don't decide, just do :-)
 */
void Alarm::Signal(const char *sensor, AlarmZone zone) {
  Serial.printf("Got alarm from sensor %s\n", sensor);

  if (zone == ZONE_FROMPEER) {
    SoundAlarm(sensor);
    return;
  }

  // A locally generated (a sensor) alarm : evaluate based on the zone and alarm state
  // Return from the function in combinations that don't throw alarms.
  if (armed == ALARM_OFF && zone != ZONE_ALWAYS)
    return;
  if (zone == ZONE_SECURE && armed == ALARM_NIGHT)
    return;

  // If we get here, we're hitting the alarm.
  SoundAlarm(sensor);
  peers->AlarmSignal(sensor, zone); // Forward alarm to peers
}

/*
 * We've decided : just start yelling.
 * Local only (don't forward to peers, this happened in an earlier phase).
 */
void Alarm::SoundAlarm(const char *sensor) {
  alert = true;
}

/*
 * This is to be called from Peers.cpp, so ZONE_FROMPEER.
 */
void Alarm::Reset(const char *module) {
  alert = false;
}

/*
 * Reset originates from the local user interface
 */
void Alarm::Reset(time_t nowts, const char *user) {
  alert = false;

  peers->AlarmReset(user);
}

/*
 * Report environmental information periodically
 */
void Alarm::loop(time_t nowts) {
}
