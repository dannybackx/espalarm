/*
 * This module queries the weather forecast
 *
 * Only one controller should perform the web queries (the one with CTOR parameter true).
 * The others can get the info from that node.
 * The code won't query more than once per 90 minutes. (See preferences.h)
 *
 * As the ESP8266 will run out of memory (RAM) converting GIF images, polling those and
 * converting them into raw format is also done on only one node. So all this imagery
 * doesn't necessarily happen on a node with an OLED.
 *
 * Other nodes get the info pushed by the "central" one via JSON.
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
#include <Peers.h>
#include <Config.h>

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

  pic = 0;
  picw = pich = 0;
  picx = 30;
  picy = 90;				// 100 descends too much

  icon_txt = icon_url = weather = pressure_trend = wind_dir = relative_humidity = 0;

  if (oled) {
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
}

/*
 * Internet query, only on the CentralNode
 */
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
  // More verbose :
  // Serial.printf("Querying %s {%s} .. ", PREF_WUNDERGROUND_API_SRV, query);

  if (! http->connect(PREF_WUNDERGROUND_API_SRV, 80)) {	// Not connected
    Serial.println("Could not connect");
    the_delay = error_delay;
    free(query); query = 0;
    return;
  }
  http->print(query);
  http->flush();

  free(query); query = 0;

  buf = (char *)malloc(buflen);
  if (buf == 0) {
    Serial.printf("Weather::PerformQuery() malloc failed\n");
    return;
  }

  buf[0] = 0;
  boolean skip = true;
  int rl = 0, nerrors = 0;

  /*
   * Cut this into two separate loops for clarity
   * This first loop skips the HTTP headers
   */
  while (skip && http && (http->connected() || http->available())) {
      String line = http->readStringUntil('\n');
      // Serial.println(line);
      if (line.length() <= 1)
        skip = false;
  }
  /*
   * Second loop reads the JSON
   */
  while (http && (http->connected() || http->available())) {
      int nb = http->read((uint8_t *)&buf[rl], buflen - rl);
      if (nb > 0) {
        rl += nb;
					// Serial.printf(" --> read %d, total %d\n", nb, rl);
	if (rl > buflen)
	  rl = buflen;
      } else if (nb < 0) {
        // Serial.printf("Read error %d, already read %d bytes\n", nb, rl);
	nerrors++;
        delay(300);

	if (nerrors > 5) {
          Serial.printf("Quit due to read error %d\n", nb);

	  free(buf);
	  buf = 0;
	  the_delay = error_delay;				// Shorter retry
	  return;
	}
      }
  }

  if (rl >= buflen) {
    Serial.println("buffer overflow");
    free(buf);
    buf = 0;
    the_delay = error_delay;				// Shorter retry
    return;
  }
  buf[rl++] = 0;

  Serial.println("ok");

  // Serial.printf("Heap (before JSON) %d\n", ESP.getFreeHeap());
  boolean fail = true;

  DynamicJsonBuffer jb;
  char *ptr = SkipHeaders(buf);
  JsonObject &root = jb.parseObject(ptr);
  if (root.success()) {
    JsonObject &current = root["current_observation"];
    if (current.success()) {
      FromPeer(current);
      fail = false;
    }
  }

  if (fail) {
    Serial.println("Failed to parse JSON");
    Serial.printf("Response received, length %d\n", rl);
    Serial.printf("\n\n%s\n\n", buf);

    the_delay = error_delay;				// Shorter retry
    free(buf); buf = 0;
    return;
  }

  free(buf);
  buf = 0;

  the_delay = normal_delay;
  changed = true;

  /*
   * Obtain and convert the corresponding image
   */
  if (gif && icon_url)
    gif->loadGif(icon_url);

  /*
   * Put this all in a shorter JSON and send to our peers.
   */
  if (peers) {
    const char *wjson = CreatePeerMessage();
    peers->SendWeather(wjson);
    free((void *)wjson);

    char msg[80], msg2[80];
    if (icon_txt)
      sprintf(msg, "Weather temp %%c °C pres %%p mb %%w km/u %%r mm, %s", icon_txt);
    else
      sprintf(msg, "Weather temp %%c °C pres %%p mb %%w km/u %%r mm");
    strfweather(msg2, sizeof(msg2), msg);
    peers->Report(msg2);
  }
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
  if (pic) {
    free(pic);
    pic = 0;
  }
}

/*
 * Loop function
 */
void Weather::loop(time_t nowts) {
  uint32_t n = (uint32_t)nowts;

  if (n >= 0 && n < 1000)			// Wait until we know the time
    return;

  if (centralNode) {
    if (last_query == 0 || (n - last_query > the_delay)) {
      PerformQuery();
      last_query = nowts;
    }
  } else if (oled && weather == 0) {		// Non-central node with display
    // By now we know the time, so query the weather node
    Peer *wn = peers->FindWeatherNode();
    if (wn) {
      // Serial.printf("Weather node is %s\n", wn->ip.toString().c_str());

      char *json = peers->CallPeer(wn, (char *)"{ \"query\" : \"weather\" }");
      // Serial.printf("Weather JSON %s\n", json);

      DynamicJsonBuffer jb;
      JsonObject &root = jb.parseObject(json);
      if (root.success()) {
        FromPeer(root);
      }

      if (json)
        free(json);

      // Arrange download of the image
      peers->ImageFromPeerBinary(wn->ip, 0, picw, pich);
    } else {
      // Serial.printf("Weather node not found\n");
    }
  }

  if (oled)
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
      strfweather(buffer[i], sizeof(buffer[i]), format[i]);

				// Serial.printf("%d %s,", i, buffer[i]);

      oled->setTextSize(1);
      oled->drawString(buffer[i], wposx[i], wposy[i]);
    }
  				// Serial.printf(")\n");
    oled->fontSize(1);
}

/*
 * Perform a strftime-like translation
 */
void Weather::strfweather(char *buffer, int buflen, const char *format) {
  char	*endp = buffer + buflen;
  char	tbuf[16];
  int	tc_1, tc_2;

  for (; *format && buffer < endp - 1; format++) {
    tbuf[0] = 0;
    if (*format != '%') {
      *buffer++ = *format;
      *buffer = 0;
      continue;
    }
    switch (*++format) {
    // Temperature
    case 'c':				// Celcius
      tc_1 = temp_c;
      tc_2 = (temp_c < 0) ? (tc_1 - temp_c) * 10 : (temp_c - tc_1) * 10;
      sprintf(tbuf, "%d.%d", tc_1, tc_2);
      break;
    case 'C':				// Fahrenheit
      tc_1 = temp_f;
      tc_2 = (tc_1 < 0) ? (tc_1 - temp_f) * 10 : (temp_f - tc_1) * 10;
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
	*buffer = 0;
      } else
        return;
    }
  }
}

/*
 * Create JSON format string
 * Caller must free memory
 */
char *Weather::CreatePeerMessage() {
  char *r = (char *)malloc(peer_message_maxlen);

  DynamicJsonBuffer jb;
  JsonObject &jo = jb.createObject();

  // Generic part of the message
  jo["weather"] = weather;				// Doubles as identifier and info
  jo["name"] = config->myName();			// identify ourselves

  // Real content
  jo["icon_url"] = icon_url;
  jo["temp_c"] = temp_c;
  jo["temp_f"] = temp_f;
  jo["relative_humidity"] = relative_humidity;
  jo["wind_kph"] = wind_kph;
  jo["wind_mph"] = wind_mph;
  jo["precip_today_metric"] = precip_today_metric;
  jo["precip_today_in"] = precip_today_in;
  jo["pressure_mb"] = pressure_mb;
  jo["pressure_in"] = pressure_in;
  jo["pressure_trend"] = pressure_trend;

  // Image info
  jo["w"] = picw;
  jo["h"] = pich;

  // Send output
  jo.printTo(r, peer_message_maxlen);

  return r;
}

/*
 * Decode the JSON we get, both from Wunderground and from peers
 */
void Weather::FromPeer(JsonObject &json) {
  temp_c = (const float)json["temp_c"];
  feelslike_c = (const float)json["feelslike_c"];
  temp_f = (const float)json["temp_f"];
  feelslike_f = (const float)json["feelslike_f"];

			// int	temp_c_a = (int)temp_c,
			// temp_c_b = (temp_c - temp_c_a) * 10;
			// Serial.printf("Current observation : %d.%d °C\n", temp_c_a, temp_c_b);
			// delay(500);

  const char *rh = json["relative_humidity"];
  if (rh) {
    if (relative_humidity) free(relative_humidity);
    relative_humidity = strdup(rh);
  }

  precip_today_metric = (const int)json["precip_today_metric"];
  precip_today_in = (const int)json["precip_today_in"];

			// Serial.printf("Humidity %s, rain %d mm %d inch\n",
			//   relative_humidity, precip_today_metric, precip_today_in);
			// delay(500);

  const char *w = json["weather"];
  if (w) {
    if (weather) free(weather);
    weather = strdup(w);
  }

  pressure_mb = (const int)json["pressure_mb"];
  pressure_in = (const int)json["pressure_in"];
  const char *p = json["pressure_trend"];
  if (p) {
    if (pressure_trend) free(pressure_trend);
    pressure_trend = strdup(p);
  }

  			// Serial.printf("Pressure %d mb %d inch, trend %s\n",
			//   pressure_mb, pressure_in, pressure_trend);
			// delay(500);

  observation_epoch = (const int)json["observation_epoch"];

  const char *wd = json["wind_dir"];
  if (wd) {
    if (wind_dir) free(wind_dir);
    wind_dir = strdup(wd);
  }

  wind_kph = (const int)json["wind_kph"];
  wind_mph = (const int)json["wind_mph"];

  const char *iurl = json["icon_url"];
  if (iurl) {
    if (icon_url) free(icon_url);
    icon_url = strdup(iurl);
  }
  const char *itxt = json["icon"];
  if (itxt) {
    if (icon_txt) free(icon_txt);
    icon_txt = strdup(itxt);
  }

  // Image
  // Don't overwrite the picw/pich variables if this is a Wunderground message.
  // This data only gets transferred if from peer to peer.
  int x;
  x = json["w"];
  if (x) picw = x;
  x = json["h"];
  if (x) pich = x;

  changed = true;
			// Serial.printf("Weather::FromPeer return\n");
}

/*
 * We store the icon here, gets passed either by Peers.cpp or LoadGif.cpp .
 */
void Weather::drawIcon(const uint16_t *icon, uint16_t width, uint16_t height) {
  // Serial.printf("Weather::drawIcon(%p, %d, %d)\n", icon, width, height);
  if (pic)
    free(pic);
  pic = (uint16_t *)icon;
  picw = width;
  pich = height;

  if (oled)
    oled->drawIcon(icon, picx, picy, width, height);
}

/*
 * Skip HTML headers
 */
char *Weather::SkipHeaders(char *buf) {
  char *r = buf, *p, *q;

  while (r && *r) {
    bool skip = false;

    // Look for next line
    p = strchr(r, '\n');
    if (p[1] == '\r')
      p += 2;	// Skip \n\r
    else
      p++;	// Just skip \n

    if (strncmp(r, "HTTP/", 5) == 0)
      skip = true;
    q = strchr(r, ':');
    if (q && q < p)
      skip = true;

    if (! skip)
      return r;

    // Go to next line
    r = p;
  }
}
