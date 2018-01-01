/*
 * Alarm controller
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
#include <ThingSpeakLogger.h>
#include <BackLight.h>
#include <Sensors.h>
#include <Alarm.h>
#include <Config.h>
#include <Peers.h>
#include <Rfid.h>

void SetupWifi();
void SetupOTA();

#define OTA_ID		"OTA-Controller"
String		ips, gws;

Config			*config;
ThingSpeakLogger	*tsl;
Sensors			*sensors;
Alarm			*_alarm;
Peers			*peers;
Rfid			*rfid;

time_t			nowts;


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
  _alarm = new Alarm();
  peers = new Peers();

  SPI.begin();
  rfid = new Rfid();

  Serial.println("Ready");

  _alarm->SetArmed(ALARM_ON);
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
  _alarm->loop(nowts);
  peers->loop(nowts);
  rfid->loop(nowts);
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
