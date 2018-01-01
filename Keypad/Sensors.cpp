/*
 * This module manages wireless sensors, e.g. Kerui PIR motion detectors or smoke detectors.
 *
 * This is not a per sensor class, it manages a list of sensors.
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
  AddSensor(SENSOR_7_ID, SENSOR_7_NAME, SENSOR_7_ZONE);
  AddSensor(SENSOR_8_ID, SENSOR_8_NAME, SENSOR_8_ZONE);
  AddSensor(SENSOR_9_ID, SENSOR_9_NAME, SENSOR_9_ZONE);
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

    // Serial.printf("Received %d (0x%08X) / %d bit, protocol %d\n",
    //   sv, sv, radio->getReceivedBitlength(), radio->getReceivedProtocol());

    radio->resetAvailable();

    for (Sensor s : sensorlist)
      if (s.id == sv) {
        _alarm->Signal(s.name, ZONE_SECURE);
	return;
      }
    Serial.printf("Sensor %d not recognized\n", sv);
  }
}
