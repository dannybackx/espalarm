#include <Oled.h>

#ifndef _CLOCK_H_
#define _CLOCK_H_

class Clock {
public:
  Clock(Oled *);
  void loop(void);

protected:

private:
  Oled *oled;
  void draw(void);
  char buffer[32];
  char hr, min, sec;
  int first;

  bool IsDST(int day, int month, int dow);
  int _isdst = 0;
  void Debug(const char *format, ...);
  time_t mySntpInit();

  int	counter;
  int	oldminute, newminute, oldhour, newhour;
  time_t	the_time;
};
#endif
