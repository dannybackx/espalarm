/*
 * Secure keypad : one that doesn't need unlock codes
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
#include <Wire.h>

extern "C" {
#include <sntp.h>
}

extern String			ips, gws;

extern Config			*config;
extern Oled			*oled;
extern Clock			*_clock;
extern BackLight		*backlight;
extern Sensors			*sensors;
extern Alarm			*_alarm;
extern Peers			*peers;
extern Rfid			*rfid;
extern Weather			*weather;
extern LoadGif			*gif;
extern boolean			in_ota;
extern int			OTAprogress;

extern time_t			nowts, boot_time;
