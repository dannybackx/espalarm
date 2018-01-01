/*
 * This module manages wireless sensors, e.g. Kerui PIR motion detectors or smoke detectors.
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

#ifndef	_WIRELESS_SENSOR_H_
#define	_WIRELESS_SENSOR_H_

#include <Alarm.h>
#include "RCSwitch.h"
#include <list>
using namespace std;

enum SensorStatus {
  SENSOR_
};

struct Sensor {
  int			id;
  char			*name;
  enum AlarmZone	zone;
};

typedef list<Sensor> SensorList;

class Sensors {
public:
  Sensors();
  ~Sensors();
  void loop(time_t);
  void AddSensor(int id, const char *name, const char *zone);

private:
  enum SensorStatus	status;
  int			radioPin;

  RCSwitch		*radio;

  list<Sensor>		sensorlist;
  AlarmZone AlarmZone2Zone(const char *);
};

#endif	/* _WIRELESS_SENSOR_H_ */
