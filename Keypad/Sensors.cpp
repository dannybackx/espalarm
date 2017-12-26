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
  sensorlist = new list<Sensor>();

  // 
  radioPin = config->GetRadioPin();
  if (radioPin < 0) {
    radio = 0;
    return;
  }

  //
  radio = new RCSwitch();
  radio->enableReceive(radioPin);

  // Add sensors predefined in secrets.h
  AddSensor(SENSOR_1_ID, SENSOR_1_NAME);
  AddSensor(SENSOR_2_ID, SENSOR_2_NAME);
  AddSensor(SENSOR_3_ID, SENSOR_3_NAME);
  AddSensor(SENSOR_4_ID, SENSOR_4_NAME);
  AddSensor(SENSOR_5_ID, SENSOR_5_NAME);
  AddSensor(SENSOR_6_ID, SENSOR_6_NAME);
}

Sensors::~Sensors() {
}

void Sensors::AddSensor(int id, const char *name) {
  if (id == 0)
    return;	// an undefined sensor

  Serial.printf("Adding sensor {%s} 0x%08x\n", name, id);

  Sensor *sp = new Sensor();
  sp->id = id;
  sp->name = (char *)name;
  sensorlist->push_back(*sp);
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

    for (Sensor s : *sensorlist)
      if (s.id == sv) {
        alarm->Signal(s.name, ZONE_SECURE);
	return;
      }
    Serial.printf("Sensor not recognized\n");
  }
}
