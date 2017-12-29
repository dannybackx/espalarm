/*
 * Alarm controller
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
#include <ArduinoOTA.h>
#include "secrets.h"
#include <ThingSpeakLogger.h>
#include <BackLight.h>
#include <Sensors.h>
#include <Clock.h>
#include <Alarm.h>
#include <Config.h>
#include <Peers.h>
#include <Rfid.h>

void SetupWifi();
void SetupOTA();

#define OTA_ID		"OTA-Controller"
String		ips, gws;

Config			*config;
Clock			*clock;
ThingSpeakLogger	*tsl;
Sensors			*sensors;
Alarm			*alarm;
Peers			*peers;
Rfid			*rfid;

time_t			nowts;


const int led_pin = D3;
int currentcolor;

void setup(void) {
				Serial.begin(115200);
				Serial.print("Starting WiFi "); 
  SetupWifi();
				Serial.printf("Set up OTA (id %s) ..", OTA_ID);
  SetupOTA();
				Serial.print(" done\nInitializing .. \n");
  config = new Config();

  tsl = new ThingSpeakLogger(TS_CHANNEL_ID, TS_WRITE_KEY);

  sensors = new Sensors();
  alarm = new Alarm();
  peers = new Peers();

  SPI.begin();
  rfid = new Rfid();

  Serial.println("Ready");
}


void loop()
{
  uint16_t	t_x, t_y, t_z;	// To store the touch coordinates
  uint16_t	d_x, d_y;
  uint8_t	pressed;

  ArduinoOTA.handle();

  nowts = now();

  tsl->loop(0);
  sensors->loop(nowts);
  alarm->loop(nowts);
  peers->loop(nowts);
  rfid->loop(nowts);
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
