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
  channel = 0;
  resolution = 8;
  freq = 5000;
  pwmrange = (uint16_t) exp2(resolution);

  SetBrightness(70);
  bright_low = 10;
  bright_high = 70;

  led_pin = pin;
  timeout = 10;
				// Initialize LED control pin
  pinMode(led_pin, OUTPUT);

#if defined(ESP32)
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(led_pin, channel);
#endif

  // Serial.printf("BackLight(pin %d)\n", led_pin);

  // Display off
  status = BACKLIGHT_NONE;
#if defined(ESP32)
  ledcWrite(channel, 0);
#else
  analogWrite(led_pin, 0);
#endif
}

void BackLight::SetStatus(BackLightStatus ns) {
  status = ns;

  if (status == BACKLIGHT_ON || status == BACKLIGHT_TEMP_ON) {
    // Serial.printf("BackLight::SetStatus %d\n", brightness);
#ifdef ESP32
    ledcWrite(channel, brightness);
#else
    analogWrite(led_pin, percentage);
#endif
  } else if (status == BACKLIGHT_NONE || status == BACKLIGHT_TEMP_OFF) {
    // Serial.printf("BackLight::SetStatus %d\n", 0);
#ifdef ESP32
    ledcWrite(channel, 0);
#else
    analogWrite(led_pin, 0);
#endif
  }

  if (status == BACKLIGHT_TEMP_ON || status == BACKLIGHT_TEMP_OFF)
    trigger_ts = now();
}

void BackLight::SetBrightness(int pctage) {
  percentage = pctage;
  brightness = percentage * pwmrange / 100;

  // Serial.printf("BackLight %d %%\n", percentage);
}

void BackLight::Trigger(time_t ts) {
  if (status == BACKLIGHT_TEMP_OFF) {
    status = BACKLIGHT_TEMP_ON;		// Change status, keep track of time, light on
    trigger_ts = ts;

#ifdef ESP32
    ledcWrite(channel, brightness);
#else
    analogWrite(led_pin, percentage);
#endif
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
 * Loop function : deal with timeout, and slowly dimming the backlight.
 */
void BackLight::loop(time_t nowts) {
  if (status == BACKLIGHT_TEMP_ON) {
    if (trigger_ts + timeout < nowts) {
#ifdef ESP32
      ledcWrite(channel, 0);
#else
      analogWrite(led_pin, 0);
#endif
      status = BACKLIGHT_TEMP_CHANGING;
      trigger_ts = 0;
      slow = 0;
    }
  }
  if (status == BACKLIGHT_TEMP_CHANGING) {		// Slowly go to bright_low
    if (percentage <= bright_low)
      status = BACKLIGHT_TEMP_OFF;			// We've reached it
    else {
      slow = (slow + 1) % 10;				// FIXME hardcoded factor
      if (slow == 0)
        percentage--;					// Slow this down ?
      SetBrightness(percentage);

#ifdef ESP32
      ledcWrite(channel, brightness);
#else
      analogWrite(led_pin, percentage);
#endif
    }
  }
}

void BackLight::touched(time_t nowts) {
  // Serial.println("BackLight::touched");

  trigger_ts = nowts;
  if (status == BACKLIGHT_TEMP_OFF || status == BACKLIGHT_TEMP_CHANGING)
    status = BACKLIGHT_TEMP_ON;
  if (status == BACKLIGHT_TEMP_ON) {
    SetBrightness(bright_high);

#ifdef ESP32
    ledcWrite(channel, brightness);
#else
    analogWrite(led_pin, percentage);
#endif
  }
}
