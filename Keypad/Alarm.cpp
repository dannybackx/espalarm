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

Alarm::Alarm(Oled *oled) {
  this->oled = oled;

  armed = ALARM_OFF;
  alert = false;
  alarmButton = 0;

  lasttime = 0;

  if (oled) {
    alarmButton = new OledButton();
    alarmButton->initButton(oled,
      120,					// x
      90,					// y
      180, 120,					// w, h
      TFT_WHITE, TFT_DARKGREEN, TFT_WHITE,	// outline, fill, text colours
      (char *)"Uit", 3);			// text, size
    alarmButton->drawButton();
  }
}

Alarm::~Alarm() {
}

void Alarm::SetArmed(const char *ss) {
  if (strcasecmp(ss, "armed") == 0)
    SetArmed(ALARM_ON);
  else if (strcasecmp(ss, "disarmed") == 0)
    SetArmed(ALARM_OFF);
  else if (strcasecmp(ss, "night") == 0)
    SetArmed(ALARM_NIGHT);
  else
    ;	// FIX ME
}

void Alarm::SetArmed(AlarmStatus s) {
  armed = s;

  if (alarmButton == 0)
    return;

  switch (s) {
  case ALARM_OFF:
    alarmButton->setFillColor(TFT_GREEN);
    alarmButton->setText("Uit");
    break;
  case ALARM_ON:
    alarmButton->setFillColor(TFT_RED);
    alarmButton->setText("Aan");
    break;
  }
}

void Alarm::SetArmed(AlarmStatus s, AlarmZone zone) {
  SetArmed(s);

  if (zone != ZONE_FROMPEER) {
    peers->AlarmSetArmed(s);
  }
}

AlarmStatus Alarm::GetArmed() {
  return armed;
}

const char *Alarm::GetArmedString() {
  switch (armed) {
    case ALARM_ON: return "armed";
    case ALARM_OFF: return "disarmed";
    case ALARM_NIGHT: return "night";
    default: return 0;
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
 *
 */
void Alarm::loop(time_t nowts) {
#if 0
  // Silly test code for the setFillColor method
  if ((nowts % 6) == 0) {
    if (alarmButton) alarmButton->setFillColor(TFT_RED);
  } else if ((nowts % 6) == 3) {
    if (alarmButton) alarmButton->setFillColor(TFT_GREEN);
  }
#endif

  if (oled) {
    uint16_t tx, ty;
    (void) oled->getTouchRaw(&tx, &ty);

    if (oled->getTouchRawZ() > 500 && alarmButton->contains(tx, 320 - ty)) {
      AlarmButtonPressed();
    } else
      AlarmButtonUnpressed();
  }
}

boolean _t = false;
int _cnt = 0;

void Alarm::AlarmButtonPressed() {
  // Do separate query to get subsecond time indication
  struct timeval tv;
  gettimeofday(&tv, NULL);

  uint32_t	ttt = tv.tv_sec * 1000000 + tv.tv_usec;

  // Debounce
  if (lasttime == 0 || ttt - lasttime > 200000) {
    lasttime = ttt;
  } else
    return;

  Serial.printf("AlarmButtonPressed(%d)\n", lasttime);
#if 0
  if (_cnt++ < 5) Serial.printf("AlarmButtonPressed(%s)\n", _t ? "true" : "false");
  _t = ! _t;
#else
  if (armed == ALARM_OFF)
    SetArmed(ALARM_ON);
  else
    SetArmed(ALARM_OFF);
#endif
}

void Alarm::AlarmButtonUnpressed() {
  if (lasttime > 0)
    Serial.printf("AlarmButtonUnpressed()\n");
  lasttime = 0;
}
