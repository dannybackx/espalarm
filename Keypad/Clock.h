/*
 * Class to display a clock on a specified place on the screen
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

#include <Oled.h>

#ifndef _CLOCK_H_
#define _CLOCK_H_

enum DstHandled {
  DST_NONE,
  DST_BUSY,
  DST_OK
};

class Clock {
public:
  Clock(Oled *);
  void loop(void);

protected:

private:
  Oled		*oled;
  void		draw(void);
  char		buffer[32];
  char		hr, min, sec;
  int		first;
  DstHandled	dstHandled;
  void HandleDST1();
  void HandleDST2();

  bool IsDST(int day, int month, int dow);
  int _isdst = 0;
  void Debug(const char *format, ...);
  time_t mySntpInit();

  int		counter;
  int		oldminute, newminute, oldhour, newhour;
  time_t	the_time;
};
#endif
