/*
 * Class to display a clock on a specified place on the screen
 * Three pieces of text can be drawn in configurable places and fonts.
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

#include <Clock.h>
#include <TimeLib.h>
#include <stdarg.h>

extern "C" {
#include <sntp.h>
#include <time.h>
}

#include <preferences.h>

Clock::Clock(Oled *oled) {
  this->oled = oled;
  hr = min = sec = 0;
  counter = 0;

  // Set up real time clock
  // Note : DST processing comes later
  (void)sntp_set_timezone(PREF_TIMEZONE);
  sntp_init();
  sntp_setservername(0, (char *)"ntp.scarlet.be");
  sntp_setservername(1, (char *)"ntp.belnet.be");

  dstHandled = DST_NONE;

  // Specify default clock
  for (int i=0; i<PREF_CLOCK_NB; i++) {
    first[i] = 1;
    format[i] = 0;
    tposx[i] = 0;
    tposy[i] = 0;
    buffer[i][0] = 0;
  }

  // Default setting : three fields

  // Time, displayed in large font
  tposx[0] = 3;
  tposy[0] = 50;
  format[0] = (char *)"%H:%M";
  font[0] = 4;
  buffer[0][0] = 0;

  // Smaller day-of-week, date
  tposx[1] = 140;
  tposy[1] = 50;
  format[1] = (char *)"%a %d/%m";
  font[1] = 1;
  buffer[1][0] = 0;

  // Smaller month-year
  tposx[2] = 140;
  tposy[2] = 75;
  format[2] = (char *)"%Y";
  font[2] = 1;
  buffer[2][0] = 0;
}

void Clock::loop() {
  // Basically try to get the time, do nothing until you have that.
  if (dstHandled != DST_OK) {
    // Try to kick us a step further
    if (dstHandled == DST_NONE)
      HandleDST1();
    else if (dstHandled == DST_BUSY)
      HandleDST2();

    // Return unless we have decent time settings
    if (dstHandled != DST_OK)
      return;
  }

  // Only do this once per second or so (given the delay 100)
  if (counter++ % 10 == 0) {
    counter = 0;

    oldminute = newminute;
    oldhour = newhour;

    the_time = sntp_get_current_timestamp();

    newhour = hour(the_time);
    newminute = minute(the_time);

    if (newminute == oldminute && newhour == oldhour) {
      return;
    }

    draw();
  }
}

void Clock::draw() {
				// Serial.printf("Clock draw(");
  for (int i=0; i<PREF_CLOCK_NB; i++)
    if (format[i] != 0 && format[i][0] != 0) { 
      oled->fontSize(font[i]);

      if (first[i] == 0) {
        oled->setTextColor(ILI9341_BLACK);
        oled->drawString(buffer[i], tposx[i], tposy[i]);
        oled->setTextColor(ILI9341_WHITE);
      }
      first[i] = 0;

      struct tm *ptm = localtime(&the_time);
      strftime(buffer[i], sizeof(buffer[i]), format[i], ptm);

				// Serial.printf("%d %s,", i, buffer[i]);

      oled->setTextSize(1);
      oled->drawString(buffer[i], tposx[i], tposy[i]);
    }
  				// Serial.printf(")\n");
    oled->fontSize(1);
}

bool Clock::IsDST(int day, int month, int dow)
{
  dow--;	// Convert this to POSIX convention (Day Of Week = 0-6, Sunday = 0)

  // January, february, and december are out.
  if (month < 3 || month > 10)
    return false;

  // April to October are in
  if (month > 3 && month < 10)
    return true;

  int previousSunday = day - dow;

  // In march, we are DST if our previous sunday was on or after the 8th.
  if (month == 3)
    return previousSunday >= 8;

  // In november we must be before the first sunday to be dst.
  // That means the previous sunday must be before the 1st.
  return previousSunday <= 0;
}

void Clock::HandleDST1() {
  time_t t;

  // Wait for a correct time, and report it

  t = sntp_get_current_timestamp();
  if (t < 0x1000)
    return;

  // DST handling
  if (IsDST(day(t), month(t), dayOfWeek(t))) {
    _isdst = 1;

    // Set TZ again
    sntp_stop();
    (void)sntp_set_timezone(PREF_TIMEZONE + 1);

    // Re-initialize/fetch
    sntp_init();
    dstHandled = DST_BUSY;
  } else {
    t = sntp_get_current_timestamp();
    dstHandled = DST_OK;
  }
}

void Clock::HandleDST2() {
  time_t t = sntp_get_current_timestamp();
  if (t > 0x1000)
      dstHandled = DST_OK;
}
