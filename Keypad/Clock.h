/*
 * Class to display a clock on a specified place on the screen
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
