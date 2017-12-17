/*
 * Test program to develop / debug the multi-screen feature with
 *
 * Copyright (c) 2017 Danny Backx
 *
 * License (MIT license):
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

// Prepare for OTA software installation
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include "secrets.h"
#include <ThingSpeakLogger.h>

#include <Oled.h>
#include <Clock.h>

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

#define OTA_ID		"OTA-TestTouch"
String		ips, gws;

Oled			oled;
Clock			*clock;
ThingSpeakLogger	*tsl;

// Size of the color selection boxes and the paintbrush size
#define BOXSIZE		40
#define PENRADIUS	2

const int led_pin = D3;
int currentcolor;

#define	NUMKEYS		3

char keyLabel[NUMKEYS][5] = {"New", "Del", "Send" };
uint16_t keyColor[NUMKEYS] = {TFT_RED, TFT_BLUE, TFT_GREEN };

// Invoke the TFT_eSPI button class and create all the button objects
OledButton key[NUMKEYS];

void setup(void) {
				Serial.begin(9600);
				Serial.print("Starting WiFi "); 
  SetupWifi();
				Serial.printf("Set up OTA (id %s) ..", OTA_ID);
  SetupOTA();
				Serial.print(" done\nInitializing .. ");
  oled = Oled();
				// Initialize LED control pin
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, 0);	// Display off

  oled.begin();

  screen1.buttonText = xxx;
  screen1.buttonHandler = yyy;
  s1 = oled.addScreen(screen1);
  s2 = oled.addScreen(screen2);
  oled.showScreen(s1);

  digitalWrite(led_pin, 1);	// Display on

  clock = new Clock(&oled);

  tsl = new ThingSpeakLogger(TS_CHANNEL_ID, TS_WRITE_KEY);
}


void loop()
{
  uint16_t	t_x, t_y, t_z;	// To store the touch coordinates
  uint16_t	d_x, d_y;
  uint8_t	pressed;

  ArduinoOTA.handle();

  oled.loop();
  clock->loop();
  tsl->loop(0);

  pressed = oled.getTouchRaw(&t_x, &t_y);
  t_z = oled.getTouchRawZ();

  if (pressed == 0 || t_z < 500) 
    return;

				  Serial.printf("X = %4d\tY = %4d\tZ = %4d\n", t_x, t_y, t_z);
  oled.fillCircle(t_x, t_y, PENRADIUS, ILI9341_RED);
}

/*
 * Prepare OTA with minimal verbosity (messages on the console)
 */
void SetupOTA() {
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(OTA_ID);
  WiFi.hostname(OTA_ID);
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

  oled.fillScreen(ILI9341_BLACK);

#if 0
  // make the color selection boxes
  oled.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_RED);
  oled.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_YELLOW);
  oled.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, ILI9341_GREEN);
  oled.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, ILI9341_CYAN);
  oled.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, ILI9341_BLUE);
  oled.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, ILI9341_MAGENTA);

  // select the current color 'red'
  oled.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
  currentcolor = ILI9341_RED;
#endif
				Serial.println("OLED ready.");
}

void s2draw(OledScreen *pscr) {
  Serial.printf("s2draw: drawing screen %s\n", pscr->name.c_str());
}
