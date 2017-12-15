/*
 * Class to display a clock on a specified place on the screen
 *
 * Copyright (c) 2017 by Danny Backx
 */
#include <Clock.h>
#include <TimeLib.h>
#include <stdarg.h>

extern "C" {
#include <sntp.h>
#include <time.h>
}

#define	MY_TIMEZONE	+1

Clock::Clock(Oled *oled) {
  this->oled = oled;
  hr = min = sec = 0;
  first = 1;
  counter = 0;

  // Set up real time clock
  // Note : DST processing comes later
  (void)sntp_set_timezone(MY_TIMEZONE);
  sntp_init();
  sntp_setservername(0, (char *)"ntp.scarlet.be");
  sntp_setservername(1, (char *)"ntp.belnet.be");

  mySntpInit();
}

void Clock::loop() {
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
  if (first == 0) {
    oled->setTextColor(ILI9341_BLACK);
    oled->drawString(buffer, 10, 50);
    oled->setTextColor(ILI9341_WHITE);
  }
  first = 0;

  // sprintf(buffer, "%02d:%02d", newhour, newminute);
  struct tm *ptm = localtime(&the_time);
  strftime(buffer, sizeof(buffer), "%a %d.%m.%Y %H:%M", ptm);

  oled->setTextSize(1);
  oled->drawString(buffer, 10, 50);
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

time_t Clock::mySntpInit() {
  time_t t;

  // Wait for a correct time, and report it

  t = sntp_get_current_timestamp();
  while (t < 0x1000) {
    delay(1000);
    t = sntp_get_current_timestamp();
  }

  // DST handling
  if (IsDST(day(t), month(t), dayOfWeek(t))) {
    _isdst = 1;

    // Set TZ again
    sntp_stop();
    (void)sntp_set_timezone(MY_TIMEZONE + 1);

    // Re-initialize/fetch
    sntp_init();
    t = sntp_get_current_timestamp();
    while (t < 0x1000) {
      delay(1000);
      t = sntp_get_current_timestamp();
    }
  } else {
    t = sntp_get_current_timestamp();
  }

  return t;
}
