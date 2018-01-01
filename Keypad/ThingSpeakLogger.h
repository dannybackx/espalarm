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

#ifndef	_THING_SPEAK_H_
#define	_THING_SPEAK_H_

#include "TimeLib.h"

class ThingSpeakLogger {
public:
  ThingSpeakLogger(const unsigned long, const char *);
  ~ThingSpeakLogger();
  void loop(time_t);
  // void changeState(int hr, int mn, int sec, int motion, int state, char *msg = NULL);

private:
  time_t lasttime;
  time_t delta;

  // ThingSpeak parameters
  const char		*write_key;
  unsigned long		channel_nr;
};

#endif	/* _THING_SPEAK_H_ */
