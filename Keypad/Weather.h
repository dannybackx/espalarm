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
#include <preferences.h>
#include <Oled.h>
#include <ArduinoJson.h>

class Weather {
public:
  Weather(boolean, Oled *);
  ~Weather();
  void loop(time_t);
  void FromPeer(JsonObject &json);
  void drawIcon(const uint16_t *icon, uint16_t width, uint16_t height);
  char *CreatePeerMessage();

private:
  void PerformQuery();
  void draw();
  void strfweather(char *buffer, int buflen, const char *format);
  char *SkipHeaders(char *buf);

  char		*query;
  WiFiClient	*http;
  Oled		*oled;

  const int buflen = 4096;
  char *buf;

  boolean	centralNode;	// This one does web queries
  boolean	changed;

  time_t	last_query;
  static const char *pattern;

  static const int	normal_delay = WU_TIME_OK * 60;	// Wait time between successful queries
  static const int	error_delay = WU_TIME_FAIL * 60;// Time to wait after error
  int			the_delay;

  // Fields from the JSON query
  char		*icon_url,
  		*weather,
  		*pressure_trend,
		*wind_dir,
		*relative_humidity;
  float		feelslike_c, feelslike_f,
  		temp_c, temp_f;
  int		wind_kph, wind_mph,
		pressure_mb, pressure_in,
		observation_epoch,
		precip_today_metric, precip_today_in;

  // Stuff that is duplicated, one per displayed zone
  int		first[PREF_WEATHER_NB];
  char		buffer[PREF_WEATHER_NB][32];
  char		*format[PREF_WEATHER_NB];	// Description of the content
  int		font[PREF_WEATHER_NB];		// Font
  uint16_t	wposx[PREF_WEATHER_NB],		// Position
		wposy[PREF_WEATHER_NB];

  //
  uint16_t	*pic, picw, pich, picx, picy;

  const int peer_message_maxlen = 400;
};

extern Weather *weather;
#endif	/* _WEATHER_H_ */
