/*
 * This module feeds data into a ThingSpeak channel periodically.
 *
 * Copyright (c) 2016, 2017 Danny Backx
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
