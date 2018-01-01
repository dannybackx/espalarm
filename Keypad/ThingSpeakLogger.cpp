/*
 * This module feeds data into a ThingSpeak channel periodically.
 *
 * Copyright (c) 2016, 2017, 2018 Danny Backx
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

#include <ThingSpeakLogger.h>
#include <Arduino.h>
#include <ThingSpeak.h>

ThingSpeakLogger::ThingSpeakLogger(const unsigned long channel, const char *wkey) {
  channel_nr = channel;
  write_key = wkey;

  lasttime = 0;
  delta = 10000L;
}

ThingSpeakLogger::~ThingSpeakLogger() {
}

/*
 * Report environmental information periodically
 */
void ThingSpeakLogger::loop(time_t nowts) {
  if (nowts > lasttime + delta) {
    ThingSpeak.writeFields(channel_nr, write_key);
  }
}
