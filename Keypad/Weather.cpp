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

const char *Weather::pattern = "GET /api/%s/conditions/q/%s/%s.json HTTP/1.1\r\n"
	"User-Agent: ESP8266/1.0 Alarm Console\r\n"
	"Accept: */*\r\n"
	"Host: %s \r\n"
	"Connection: close\r\n"
	"\r\n";

Weather::Weather(boolean doit) {
  Serial.printf("Weather ctor(%s)\n", doit ? "true" : "false");
  centralNode = doit;
  the_delay = normal_delay;
  last_query = 0;
  http = 0;
  query = 0;
}

void Weather::PerformQuery() {
  if (query == 0) {
    query = (char *)malloc(strlen(WUNDERGROUND_API_KEY) + strlen(WUNDERGROUND_COUNTRY)
      + strlen(WUNDERGROUND_CITY) + strlen(pattern) + strlen(WUNDERGROUND_API_SRV));
    sprintf(query, pattern, WUNDERGROUND_API_KEY, WUNDERGROUND_COUNTRY,
      WUNDERGROUND_CITY, WUNDERGROUND_API_SRV);
    // Serial.println(query);
  }

  if (http == 0)
    http = new WiFiClient();

  Serial.printf("Querying %s ..\n", WUNDERGROUND_API_SRV);

  if (! http->connect(WUNDERGROUND_API_SRV, 80)) {	// Not connected
    Serial.println("Could not connect to wunderground");;
    the_delay = error_delay;
    return;
  }
  http->print(query);
  http->flush();

  buf = (char *)malloc(buflen);
  boolean skip = true;
  int rl = 0;
  while (http->connected() || http->available()) {
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
    delay(100);
  }

  http->stop();
  delete http;
  http = 0;

  if (rl >= buflen) {
    Serial.println("buffer overflow");
    return;
  }
  buf[rl++] = 0;
  Serial.printf("Response received, length %d\n", rl);
  Serial.printf("\n\n%s\n\n", buf);
  // return;

  DynamicJsonBuffer jb;
  JsonObject &root = jb.parseObject(buf);
  if (! root.success()) {
    Serial.println("Failed to parse JSON");
    return;
  }

  JsonObject &current = root["current_observation"];
  if (! current.success()) {
    Serial.println("Failed to parse current in JSON");
    return;
  }
  const float temp_c = current["temp_c"];
  const char *temp_cs = current["temp_c"];
  const char *hum = current["relative_humidity"];
  const char *wth = current["weather"];
  const char *pressure = current["pressure_mb"];
  const char *ob_time = current["observation_time_rfc822"];

  // Serial.printf("Current observation : %f °C hum %s, pressure %s\n", temp_c, hum, pressure);
  // Serial.printf("Weather %s\n", wth);
  // Serial.printf("Time %s\n", ob_time);

  the_delay = normal_delay;
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
int cnt = 5;
void Weather::loop(time_t nowts) {
  uint32_t n = (uint32_t)nowts;

  if (centralNode) {
    if (n >= 0 && n < 1000)
      return;
    if (last_query == 0 || (n - last_query > the_delay)) {
      PerformQuery();
      last_query = nowts;
    }
  }
}
