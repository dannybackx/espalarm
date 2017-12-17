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
