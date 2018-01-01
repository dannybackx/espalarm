/*
 * This module sounds a siren or flashes a light
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

#ifndef	_SIREN_H_
#define	_SIREN_H_

enum SirenStatus {
  ALARM_OFF,
  ALARM_ON,
  // ??
};

class Siren {
public:
  Siren();
  ~Siren();
  void loop(time_t);
  void SoundSiren(const char *sensor);

private:
  enum SirenStatus	state;

  int siren_pin;
};
#endif	/* _SIREN_H_ */
