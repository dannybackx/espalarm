/*
 * This module controls the TFT backlight
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
#include <BackLight.h>

BackLight::BackLight(int pin) {
  led_pin = pin;

  SetBrightness(70);
  bright_low = 10;
  bright_high = 70;
  timeout = 10;
				// Initialize LED control pin
  pinMode(led_pin, OUTPUT);
  analogWrite(led_pin, 0);	// Display off

  status = BACKLIGHT_NONE;
}

void BackLight::SetStatus(BackLightStatus ns) {
  status = ns;

  if (status == BACKLIGHT_ON || status == BACKLIGHT_TEMP_ON)
    analogWrite(led_pin, brightness);
  else if (status == BACKLIGHT_NONE || status == BACKLIGHT_TEMP_OFF)
    analogWrite(led_pin, 0);

  if (status == BACKLIGHT_TEMP_ON || status == BACKLIGHT_TEMP_OFF)
    trigger_ts = now();
}

void BackLight::SetBrightness(int pctage) {
  percentage = pctage;
  brightness = percentage * PWMRANGE / 100;
}

void BackLight::Trigger(time_t ts) {
  if (status == BACKLIGHT_TEMP_OFF) {
    status = BACKLIGHT_TEMP_ON;		// Change status, keep track of time, light on
    trigger_ts = ts;

    analogWrite(led_pin, brightness);
  } else if (status == BACKLIGHT_TEMP_ON) {
    trigger_ts = ts;			// Only update our time
  } else {
    // Undefined
  }
}

void BackLight::SetTimeout(int s) {
  timeout = s;
}

/*
 * Report environmental information periodically
 */
void BackLight::loop(time_t nowts) {
  if (status == BACKLIGHT_TEMP_ON) {
    if (trigger_ts + timeout < nowts) {
      analogWrite(led_pin, 0);
      status = BACKLIGHT_TEMP_CHANGING;
      trigger_ts = 0;
    }
  }
  if (status == BACKLIGHT_TEMP_CHANGING) {		// Slowly go to bright_low
    if (percentage <= bright_low)
      status = BACKLIGHT_TEMP_OFF;			// We've reached it
    else {
      percentage--;					// Slow this down ?
      SetBrightness(percentage);
      analogWrite(led_pin, brightness);
    }
  }
}

void BackLight::touched(time_t nowts) {
  trigger_ts = nowts;
  if (status == BACKLIGHT_TEMP_OFF || status == BACKLIGHT_TEMP_CHANGING)
    status = BACKLIGHT_TEMP_ON;
  if (status == BACKLIGHT_TEMP_ON) {
    SetBrightness(bright_high);
    analogWrite(led_pin, brightness);
  }
}
