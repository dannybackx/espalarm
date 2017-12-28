/*
 * This module manages wireless sensors, e.g. Kerui PIR motion detectors or smoke detectors.
 *
 * This is not a per sensor class, it manages a list of sensors.
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

#include <Arduino.h>
#include <Sensors.h>
#include <secrets.h>
#include <Config.h>
#include <Alarm.h>

#include <list>
using namespace std;

#include <RCSwitch.h>

Sensors::Sensors() {
  // 
  radioPin = config->GetRadioPin();
#if defined(ESP8266)
  if (radioPin == D0) {
    // interrupts can be attached to any GPIO except GPIO16, so not D0 or A0.
    Serial.println("Not possible to use GPIO16 (D0) for the radio on ESP8266");
    return;
  }
#endif
  if (radioPin < 0) {
    radio = 0;
    return;
  }

  //
  radio = new RCSwitch();
  radio->enableReceive(radioPin);

  // Add sensors predefined in secrets.h
  AddSensor(SENSOR_1_ID, SENSOR_1_NAME, SENSOR_1_ZONE);
  AddSensor(SENSOR_2_ID, SENSOR_2_NAME, SENSOR_2_ZONE);
  AddSensor(SENSOR_3_ID, SENSOR_3_NAME, SENSOR_3_ZONE);
  AddSensor(SENSOR_4_ID, SENSOR_4_NAME, SENSOR_4_ZONE);
  AddSensor(SENSOR_5_ID, SENSOR_5_NAME, SENSOR_5_ZONE);
  AddSensor(SENSOR_6_ID, SENSOR_6_NAME, SENSOR_6_ZONE);
}

Sensors::~Sensors() {
}

AlarmZone Sensors::AlarmZone2Zone(const char *zone) {
  AlarmZone r = ZONE_NONE;

  if (strcasecmp(zone, "secure") == 0) r = ZONE_SECURE;
  else if (strcasecmp(zone, "perimeter") == 0) r = ZONE_PERIMETER;
  else if (strcasecmp(zone, "always") == 0) r = ZONE_ALWAYS;

  return r;
}

void Sensors::AddSensor(int id, const char *name, const char *zone) {
  if (id == 0)
    return;	// an undefined sensor

  Serial.printf("Adding sensor {%s} 0x%08x\n", name, id);

  Sensor *sp = new Sensor();
  sp->id = id;
  sp->name = (char *)name;
  sp->zone = AlarmZone2Zone(zone);
  sensorlist.push_back(*sp);
}

/*
 * Report environmental information periodically
 */
void Sensors::loop(time_t nowts) {
  int sv;

  if (radio && radio->available()) {
    sv = radio->getReceivedValue();

    Serial.printf("Received %d (0x%08X) / %d bit, protocol %d\n",
      sv, sv, radio->getReceivedBitlength(), radio->getReceivedProtocol());

    radio->resetAvailable();

    for (Sensor s : sensorlist)
      if (s.id == sv) {
        alarm->Signal(s.name, ZONE_SECURE);
	return;
      }
    Serial.printf("Sensor not recognized\n");
  }
}
