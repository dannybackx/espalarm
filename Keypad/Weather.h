/*
 * This module queries the weather forecast
 *
 * Copyright (c) 2018 Danny Backx
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

#ifndef	_WEATHER_H_
#define	_WEATHER_H_

#include <WiFiClient.h>

class Weather {
public:
  Weather(boolean);
  ~Weather();
  void loop(time_t);

private:
  void PerformQuery();

  char		*query;
  WiFiClient	*http;

  const int buflen = 4096;
  char *buf;

  boolean	centralNode;	// This one does web queries

  time_t	last_query;
  static const char *pattern;

  static const int	normal_delay = 75 * 60;		// A bit more than an hour between queries
  static const int	error_delay = 20 * 60;		// Time to wait after error
  int			the_delay;
};
#endif	/* _WEATHER_H_ */
