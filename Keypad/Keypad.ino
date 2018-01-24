/*
 * Secure keypad : one that doesn't need unlock codes
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

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "secrets.h"
#include <BackLight.h>
#include <Sensors.h>
#include <Oled.h>
#include <Clock.h>
#include <Alarm.h>
#include <Config.h>
#include <Peers.h>
#include <Rfid.h>
#include <Weather.h>
#include <LoadGif.h>

extern "C" {
#include <sntp.h>
}

void s1b1(struct OledScreen *scr, int button);
void s1b2(struct OledScreen *scr, int button);
void s1b3(struct OledScreen *scr, int button);
void s2b1(struct OledScreen *scr, int button);
void s2b2(struct OledScreen *scr, int button);
void s2b3(struct OledScreen *scr, int button);
void s1draw(OledScreen *pscr);
void s2draw(OledScreen *pscr);

String xxx[] = { "yes", "no", "maybe" };

void (*yyy[])(struct OledScreen *, int) = { s1b1, s1b2, s1b3, NULL };
void (*zzz[])(struct OledScreen *, int) = { s2b1, s2b2, s2b3, NULL };

OledScreen screen1 = {
  "home",				// name
  0,					// number
  3,					// #buttons
  0, // { "yes", "no", "maybe" },
  0, // { s1b1, s1b2, s1b3 },
  s1draw
};
OledScreen screen2 = {
  "detail",
  0,
  3,
  0, 
  0,
  s2draw
};
int	s1, s2;

void SetupWifi();
void SetupOTA();

#define OTA_ID		"OTA-KeypadSecure"
String		ips, gws;

Config			*config;
Oled			*oled;
Clock			*_clock = 0;
BackLight		*backlight = 0;
Sensors			*sensors = 0;
Alarm			*_alarm = 0;
Peers			*peers = 0;
Rfid			*rfid = 0;
Weather			*weather = 0;
LoadGif			*gif = 0;

time_t			nowts;

// Size of the color selection boxes and the paintbrush size
#define BOXSIZE		40
#define PENRADIUS	2

#define	NUMKEYS		3

char keyLabel[NUMKEYS][5] = {"New", "Del", "Send" };
uint16_t keyColor[NUMKEYS] = {TFT_RED, TFT_BLUE, TFT_GREEN };

// Invoke the TFT_eSPI button class and create all the button objects
OledButton key[NUMKEYS];

void setup(void) {
				Serial.begin(115200);
				Serial.println("\nAlarm Controller (c) 2017, 2018 by Danny Backx");
				Serial.printf("Free heap : %d\n", ESP.getFreeHeap());
				Serial.print("Starting WiFi "); 
  SetupWifi();
				Serial.printf("Set up OTA (id %s) ..", OTA_ID);
  SetupOTA();
				Serial.print(" done\nInitializing .. \n");
  config = new Config();
  Serial.printf("My name is %s, have :", config->myName());
  if (config->haveOled()) Serial.print(" oled");
  if (config->haveRadio()) Serial.print(" radio");
  if (config->haveRfid()) Serial.print(" rfid");
  if (config->haveWeather()) Serial.print(" weather");
  Serial.println();

  if (config->haveOled()) {
    oled = new Oled();
    oled->begin();
    backlight = new BackLight(config->GetOledLedPin());	// led_pin, D3 is GPIO0 on D1 mini
    backlight->SetStatus(BACKLIGHT_TEMP_ON);	// Display on
    backlight->SetTimeout(10);			// Hardcoded timeout in seconds

    screen1.buttonText = xxx;
    screen1.buttonHandler = yyy;
    s1 = oled->addScreen(screen1);
    s2 = oled->addScreen(screen2);
    oled->showScreen(s1);

    gif = new LoadGif(oled);
  }

  _clock = new Clock(oled);

  // We always have a local weather module if we have an OLED
  // Only one of us actually does wunderground queries
  weather = new Weather(config->haveWeather(), oled);

  if (config->haveRadio())
    sensors = new Sensors();

  if (config->haveRfid()) {
    SPI.begin();
    rfid = new Rfid();
  }

  _alarm = new Alarm();
  peers = new Peers();

  Serial.println("Ready");
}


void loop()
{
  uint16_t	t_x, t_y, t_z;	// To store the touch coordinates
  uint16_t	d_x, d_y;
  uint8_t	pressed;

  ArduinoOTA.handle();

#ifdef ESP32
  nowts = time(0);
#else
  nowts = sntp_get_current_timestamp();
#endif

  if (weather) weather->loop(nowts);
  if (config->haveOled()) {
    oled->loop(nowts);
    _clock->loop(nowts);
    backlight->loop(nowts);
    gif->loop(nowts);
  }

  if (sensors) sensors->loop(nowts);
  _alarm->loop(nowts);
  peers->loop(nowts);
  if (rfid) rfid->loop(nowts);

  if (config->haveOled()) {
    pressed = oled->getTouchRaw(&t_x, &t_y);
    t_z = oled->getTouchRawZ();

    if (pressed == 0 || t_z < 500) 
      return;

//				  Serial.printf("X = %4d\tY = %4d\tZ = %4d\n", t_x, t_y, t_z);
    oled->fillCircle(t_x, t_y, PENRADIUS, ILI9341_RED);

  }
}

/*
 * Prepare OTA with minimal verbosity (messages on the console)
 */
void SetupOTA() {
  ArduinoOTA.setHostname(OTA_ID);
#ifdef ESP32
  ArduinoOTA.setPort(3232);
  WiFi.setHostname(OTA_ID);
#else
  ArduinoOTA.setPort(8266);
  WiFi.hostname(OTA_ID);
#endif
  ArduinoOTA.begin();
}

struct mywifi {
  const char *ssid, *pass;
} mywifi[] = {
#ifdef MY_SSID_1
  { MY_SSID_1, MY_WIFI_PASSWORD_1 },
#endif
#ifdef MY_SSID_2
  { MY_SSID_2, MY_WIFI_PASSWORD_2 },
#endif
#ifdef MY_SSID_3
  { MY_SSID_3, MY_WIFI_PASSWORD_3 },
#endif
#ifdef MY_SSID_4
  { MY_SSID_4, MY_WIFI_PASSWORD_4 },
#endif
  { NULL, NULL}
};

void SetupWifi() {
  int ix;
					// Try to connect to WiFi
  WiFi.mode(WIFI_STA);
					// Loop over known networks
  int wcr = WL_IDLE_STATUS;
  for (ix = 0; wcr != WL_CONNECTED && mywifi[ix].ssid != NULL; ix++) {
    int wifi_tries = 5;

    Serial.printf("\nTrying %s ", mywifi[ix].ssid);
    while (wifi_tries-- > 0) {
      Serial.print(".");
      WiFi.begin(mywifi[ix].ssid, mywifi[ix].pass);
      wcr = WiFi.waitForConnectResult();
      if (wcr == WL_CONNECTED)
        break;
      delay(250);
    }
  }
					// This fails if include file not read
  if (mywifi[0].ssid == NULL) {
    Serial.println("Configuration problem : we don't know any networks");
  }
					// Aw crap
  // Reboot if we didn't manage to connect to WiFi
  if (wcr != WL_CONNECTED) {
    Serial.println("Not connected -> reboot");
    delay(2000);
    ESP.restart();
  }
					// Report success
  IPAddress ip = WiFi.localIP();
  ips = ip.toString();
  IPAddress gw = WiFi.gatewayIP();
  gws = gw.toString();
  Serial.printf(" -> IP %s gw %s\n", ips.c_str(), gws.c_str());
}

void s1b1(struct OledScreen *scr, int button) {
  Serial.printf("s1b1: screen %s button %d %s\n", scr->name.c_str(), button, scr->buttonText[button].c_str());
}

void s1b2(struct OledScreen *scr, int button) {
  Serial.printf("s1b2: screen %s button %d %s\n", scr->name.c_str(), button, scr->buttonText[button].c_str());
}

void s1b3(struct OledScreen *scr, int button) {
  Serial.printf("s1b3: screen %s button %d %s\n", scr->name.c_str(), button, scr->buttonText[button].c_str());
}

void s2b1(struct OledScreen *scr, int button) {
  Serial.printf("s2b1: screen %s button %d %s\n", scr->name.c_str(), button, scr->buttonText[button].c_str());
}

void s2b2(struct OledScreen *scr, int button) {
  Serial.printf("s2b2: screen %s button %d %s\n", scr->name.c_str(), button, scr->buttonText[button].c_str());
}

void s2b3(struct OledScreen *scr, int button) {
  Serial.printf("s2b3: screen %s button %d %s\n", scr->name.c_str(), button, scr->buttonText[button].c_str());
}

void s1draw(OledScreen *pscr) {
  Serial.printf("s1draw: drawing screen %s\n", pscr->name.c_str());

  oled->fillScreen(ILI9341_BLACK);

				Serial.println("OLED ready.");
}

void s2draw(OledScreen *pscr) {
  Serial.printf("s2draw: drawing screen %s\n", pscr->name.c_str());
}

extern "C" {
  int heap() {
    return ESP.getFreeHeap();
  }
}
