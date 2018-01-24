/*
 * This module queries the weather forecast
 *
 * Only one controller should perform the web queries (the one with CTOR parameter true).
 * The others can get the info from that node.
 * The code won't query more than once per 90 minutes.
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

#include <Arduino.h>
#include <Weather.h>
#include <secrets.h>
#include <preferences.h>
#include <ArduinoJson.h>
#include <Oled.h>

#include <LoadGif.h>

const char *Weather::pattern = "GET /api/%s/conditions/q/%s/%s.json HTTP/1.1\r\n"
	"User-Agent: ESP8266-ESP32 Alarm Console/1.0\r\n"
	"Accept: */*\r\n"
	"Host: %s \r\n"
	"Connection: close\r\n"
	"\r\n";

/*
 * Constructor
 * The parameter says whether this module is the one doing internet queries.
 * (Only one of the modules queries wunderground.com, the others share the info.)
 */
Weather::Weather(boolean doit, Oled *oled) {
  Serial.printf("Weather ctor(%s)\n", doit ? "true" : "false");
  centralNode = doit;
  this->oled = oled;

  the_delay = normal_delay;
  last_query = 0;
  http = 0;
  query = 0;
  changed = false;

  // Specify default clock
  for (int i=0; i<PREF_WEATHER_NB; i++) {
    first[i] = 1;
    format[i] = 0;
    wposx[i] = wposy[i] = 0;
    buffer[i][0] = 0;
  }

  // Default setting : three fields

  // Temperature, displayed in large font
  wposx[0] = 120;
  wposy[0] = 100;
  format[0] = (char *)"%c °C";
  font[0] = 4;
  buffer[0][0] = 0;

  // Smaller : pressure, wind speed, rain
  wposx[1] = 5;
  wposy[1] = 140;
  format[1] = (char *)"%w km/u %p mb %r mm";
  font[1] = 1;
  buffer[1][0] = 0;

  // Smaller : wind speed
  wposx[2] = 55;
  wposy[2] = 120;
  // format[2] = (char *)"%w km/u";
  font[2] = 1;
  buffer[2][0] = 0;
}

void Weather::PerformQuery() {
  if (query == 0) {
    query = (char *)malloc(strlen(WUNDERGROUND_API_KEY) + strlen(WUNDERGROUND_COUNTRY)
      + strlen(WUNDERGROUND_CITY) + strlen(pattern) + strlen(PREF_WUNDERGROUND_API_SRV));
    sprintf(query, pattern, WUNDERGROUND_API_KEY, WUNDERGROUND_COUNTRY,
      WUNDERGROUND_CITY, PREF_WUNDERGROUND_API_SRV);
    // Serial.println(query);
  }

  if (http == 0)
    http = new WiFiClient();

  Serial.printf("Querying %s .. ", PREF_WUNDERGROUND_API_SRV);

  if (! http->connect(PREF_WUNDERGROUND_API_SRV, 80)) {	// Not connected
    Serial.println("Could not connect");
    the_delay = error_delay;
    return;
  }
  http->print(query);
  http->flush();

  free(query); query = 0;

  buf = (char *)malloc(buflen);
  boolean skip = true;
  int rl = 0;
  while (http && (http->connected() || http->available())) {
    if (skip) {
      String line = http->readStringUntil('\n');
      if (line.length() <= 1)
        skip = false;
    } else {
      int nb = http->read((uint8_t *)&buf[rl], buflen - rl);
      if (nb > 0) {
        rl += nb;
	if (rl > buflen)
	  rl = buflen;
      } else if (nb < 0) {
        Serial.println("Read error");
      }
    }
    delay(300);
  }
#if 0
  http->stop();
  delete http;
  http = 0;
#endif
  if (rl >= buflen) {
    Serial.println("buffer overflow");
    return;
  }
  buf[rl++] = 0;

  Serial.println("ok");

  // Serial.printf("Heap (before JSON) %d\n", ESP.getFreeHeap());

  DynamicJsonBuffer jb;
  JsonObject &root = jb.parseObject(buf);
  if (! root.success()) {
    Serial.println("Failed to parse JSON");
    Serial.printf("Response received, length %d\n", rl);
    Serial.printf("\n\n%s\n\n", buf);
    return;
  }

  JsonObject &current = root["current_observation"];
  if (! current.success()) {
    Serial.println("Failed to parse current in JSON");
    return;
  }

  temp_c = (const float)current["temp_c"];
  feelslike_c = (const float)current["feelslike_c"];
  temp_f = (const float)current["temp_f"];
  feelslike_f = (const float)current["feelslike_f"];

  relative_humidity = (char *)(const char *)current["relative_humidity"];
  precip_today_metric = (const int)current["precip_today_metric"];
  precip_today_in = (const int)current["precip_today_in"];

  weather = strdup((const char *)current["weather"]);
  icon_url = strdup((const char *)current["icon_url"]);

  pressure_mb = (const int)current["pressure_mb"];
  pressure_in = (const int)current["pressure_in"];
  pressure_trend = strdup((const char *)current["pressure_trend"]);

  observation_epoch = (const int)current["observation_epoch"];

  wind_dir = strdup((const char *)current["wind_dir"]);
  wind_kph = (const int)current["wind_kph"];
  wind_mph = (const int)current["wind_mph"];

  int	temp_c_a = (int)temp_c,
  	temp_c_b = (temp_c - temp_c_a) * 10;
  Serial.printf("Current observation : %d.%d °C, pressure %d, wind %d km/h\n",
  	temp_c_a, temp_c_b, pressure_mb, wind_kph);
  Serial.printf("Weather %s, pic %s\n", weather, icon_url);

  // Serial.printf("Weather %s\n", wth);
  // Serial.printf("Time %s\n", ob_time);

  the_delay = normal_delay;

  free(buf);
  buf = 0;

  changed = true;

  // Serial.printf("Heap (after JSON) %d\n", ESP.getFreeHeap());
  if (gif) gif->loadGif(icon_url);
}

Weather::~Weather() {
  if (buf) {
    free(buf);
    buf = 0;
  }
  if (query) {
    free(query);
    query = 0;
  }
  if (http) {
    delete http;
    http = 0;
  }
}

/*
 * Loop function : deal with timeout, and slowly dimming the backlight.
 */
void Weather::loop(time_t nowts) {
  uint32_t n = (uint32_t)nowts;

  if (centralNode) {
    if (n >= 0 && n < 1000)			// Wait until we know the time
      return;
    if (last_query == 0 || (n - last_query > the_delay)) {
      PerformQuery();
      last_query = nowts;
    }
  }

  draw();
}

void Weather::draw() {
  if (! changed)
    return;
  if (oled == 0)
    return;

  changed = false;
				// Serial.printf("Weather draw(");
  for (int i=0; i<PREF_WEATHER_NB; i++)
    if (format[i] != 0 && format[i][0] != 0) { 
      oled->fontSize(font[i]);

      if (first[i] == 0) {
        oled->setTextColor(ILI9341_BLACK);
        oled->drawString(buffer[i], wposx[i], wposy[i]);
        oled->setTextColor(ILI9341_WHITE);
      }
      first[i] = 0;

      // Print the weather info format-based : we don't have a function for that
      strwtime(buffer[i], sizeof(buffer[i]), format[i]);

				// Serial.printf("%d %s,", i, buffer[i]);

      oled->setTextSize(1);
      oled->drawString(buffer[i], wposx[i], wposy[i]);
    }
  				// Serial.printf(")\n");
    oled->fontSize(1);
}

/*
 * Perform a strftime-like translation
 *
  char		*icon_url,
  		*weather,
  		*pressure_trend,
		*wind_dir,
		*relative_humidity;
  float		feelslike_c,
  		temp_c;
  int		wind_kph,
		pressure_mb,
		observation_epoch,
		precip_today_metric;
 */
void Weather::strwtime(char *buffer, int buflen, const char *format) {
  char	*endp = buffer + buflen;
  char	tbuf[16];
  int	tc_1, tc_2;

  for (; *format && buffer < endp - 1; format++) {
    tbuf[0] = 0;
    if (*format != '%') {
      *buffer++ = *format;
      continue;
    }
    switch (*++format) {
    // Temperature
    case 'c':				// Celcius
      tc_1 = temp_c;
      tc_2 = (temp_c - tc_1) * 10;
      sprintf(tbuf, "%d.%d", tc_1, tc_2);
      break;
    case 'C':				// Fahrenheit
      tc_1 = temp_f;
      tc_2 = (temp_f - tc_1) * 10;
      sprintf(tbuf, "%d.%d", tc_1, tc_2);
      break;
    // Humidity
    case 'h':
      sprintf(tbuf, "%s", relative_humidity);
      break;
    // Wind
    case 'w':				// kilometers per hour
      sprintf(tbuf, "%d", wind_kph);
      break;
    case 'W':				// miles per hour
      sprintf(tbuf, "%d", wind_mph);
      break;
    // Rain (precipitation)
    case 'r':				// millimeters
      sprintf(tbuf, "%d", precip_today_metric);
      break;
    case 'R':				// inches
      sprintf(tbuf, "%d", precip_today_in);
      break;
    // Pressure
    case 'p':				// millibar
      sprintf(tbuf, "%d", pressure_mb);
      break;
    case 'P':				// inch
      sprintf(tbuf, "%d", pressure_in);
      break;
    default:
      tbuf[0] = '%';
      tbuf[1] = *format;
      tbuf[1] = 0;
      break;
    }

    int	i = strlen(tbuf);
    if (i) {
      if (buffer + i < endp - 1) {
        strcpy(buffer, tbuf);
	buffer += i;
      } else
        return;
    }
  }
}
