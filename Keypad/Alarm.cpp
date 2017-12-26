/*
 * This module manages Alarm state and signaling
 *	Alarms need to be passed to peer controllers
 *	Alarms passed from peer controllers don't need to be forwarded
 *	Sirens (which may be flash lights as well as audible devices) need triggering.
 *	We could potentially connect/messsage to a smartphone app
 *	We could send e-mail
 *	User interface (the Oled modules) needs to show this as well
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

#include <Arduino.h>
#include <Alarm.h>
#include <Peers.h>
#include <secrets.h>

Alarm::Alarm() {
  state = ALARM_OFF;
}

Alarm::~Alarm() {
}

void Alarm::SetState(AlarmStatus s) {
  state = s;
}

void Alarm::SetState(AlarmStatus s, AlarmZone zone) {
  state = s;
  if (zone != ZONE_FROMPEER) {
    peers->AlarmSetState(s);
  }
}

/*
 * Still to decide based on zone
 * However, if ZONE_FROMPEER then don't forward to peers, don't decide, just do :-)
 */
void Alarm::Signal(const char *sensor, AlarmZone zone) {
  Serial.printf("Got alarm from sensor %s\n", sensor);

  if (zone != ZONE_FROMPEER) {
    // Evaluate based on the zone

    // Forward alarm to peers
    peers->AlarmSignal(sensor, zone);
  }

  // Now we should start signalling
  SoundAlarm(sensor);
}

/*
 * We've decided : just start yelling.
 * Local only (don't forward to peers, this was in a prior phase).
 */
void Alarm::SoundAlarm(const char *sensor) {
}

/*
 * Report environmental information periodically
 */
void Alarm::loop(time_t nowts) {
}
