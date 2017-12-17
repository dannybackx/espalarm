/*
 * This module controls the TFT backlight
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
#include <BackLight.h>

BackLight::BackLight(int pin) {
  led_pin = pin;
				// Initialize LED control pin
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, 0);	// Display off

  status = BACKLIGHT_NONE;
}

BackLight::~BackLight() {
}

void BackLight::SetStatus(BackLightStatus ns) {
  status = ns;

  if (status == BACKLIGHT_ON || status == BACKLIGHT_TEMP_ON)
    digitalWrite(led_pin, 1);
  else if (status == BACKLIGHT_NONE || status == BACKLIGHT_TEMP_OFF)
    digitalWrite(led_pin, 0);

  if (status == BACKLIGHT_TEMP_ON || status == BACKLIGHT_TEMP_OFF)
    trigger_ts = now();
}

void BackLight::Trigger(time_t ts) {
  if (status == BACKLIGHT_TEMP_OFF) {
    status = BACKLIGHT_TEMP_ON;		// Change status, keep track of time, light on
    trigger_ts = ts;

    digitalWrite(led_pin, 1);
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
    if (trigger_ts + timeout * 1000 < nowts) {		// Milliseconds vs seconds
      digitalWrite(led_pin, 0);
      status = BACKLIGHT_TEMP_OFF;
      trigger_ts = 0;
    }
  }
}
