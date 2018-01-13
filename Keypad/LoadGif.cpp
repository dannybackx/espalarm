/*
 * This module loads a GIF and calls libnsgif to convert into raw bitmap format
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
#include "libnsgif.h"
#include <LoadGif.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <ESPWiFi.h>
#endif
#include <WiFiClient.h>

static void *bitmap_create(int width, int height);
static void bitmap_destroy(void *bitmap);
static void bitmap_set_opaque(void *bitmap, bool opaque);
static bool bitmap_test_opaque(void *bitmap);
static unsigned char *bitmap_get_buffer(void *bitmap);
static void bitmap_modified(void *bitmap);

LoadGif::LoadGif() {
  http = 0;

  bitmap_callbacks.bitmap_create = bitmap_create;
  bitmap_callbacks.bitmap_destroy = bitmap_destroy;
  bitmap_callbacks.bitmap_get_buffer = bitmap_get_buffer;
  bitmap_callbacks.bitmap_set_opaque = bitmap_set_opaque;
  bitmap_callbacks.bitmap_test_opaque = bitmap_test_opaque;
  bitmap_callbacks.bitmap_modified = bitmap_modified;
}

LoadGif::~LoadGif() {
}

void LoadGif::loadGif(const char *url) {
  Serial.printf("LoadGif(%s)\n", url);

  gif_animation gif;
  size_t size;
  gif_result code;
  char *data;

  Serial.printf("Before GIF : heap %d\n", ESP.getFreeHeap());
  gif_create(&gif, &bitmap_callbacks);
  Serial.printf("Before download : heap %d\n", ESP.getFreeHeap());

  // Download it
  data = loadGif(url, &size);
  Serial.printf("After download : heap %d\n", ESP.getFreeHeap());
  if (data == 0)
    return;

#if 1
  Serial.print("Begin decoding .. "); delay(250);
  do {
    code = gif_initialise(&gif, size, (unsigned char *)data);
    if (code != GIF_OK && code != GIF_WORKING) {
      Serial.printf("LoadGif(%s) failed : %d\n", url, code);
      return;
    }
  } while (code != GIF_OK);
#if 0
  Serial.print(" (phase 2 disabled)");
#else
  Serial.printf(" --> phase 2 .. (has %d frames) ", gif.frame_count);

#if 0
  for (int i=0; i != gif.frame_count; i++) {
    unsigned char *image;

    code = gif_decode_frame(&gif, 0);	// Only first frame
    if (code != GIF_OK)
      Serial.printf("gif error, code %d\n", code);
      image = (unsigned char *)gif.frame_image;

      for (int row=0; row != gif.height; row++)
        for (int col=0; col != gif.width; col++) {
	  size_t z = (row * gif.width + col) * 4;
	  uint16_t x = image[z] << 11 + image[z+1] << 5 + image[z+2];
	  pixels[row * gif.width + col] = x;
	}
  }
#else
{
    // Only one frame
    unsigned char *image;

    code = gif_decode_frame(&gif, 0);	// Only first frame
    if (code != GIF_OK)
      Serial.printf("gif error, code %d\n", code);
      image = (unsigned char *)gif.frame_image;

      for (int row=0; row != gif.height; row++)
        for (int col=0; col != gif.width; col++) {
	  size_t z = (row * gif.width + col) * 4;
	  uint16_t x = image[z] << 11 + image[z+1] << 5 + image[z+2];
	  pixels[row * gif.width + col] = x;
	}
}
#endif

#endif
  Serial.println("done");
  Serial.printf("After GIF decode : heap %d\n", ESP.getFreeHeap());

#endif
  gif_finalise(&gif);
  free(data);			// Frees the buffer allocated in loadGif()
}

void LoadGif::loop(time_t) {
}

#define BYTES_PER_PIXEL 2
#define MAX_IMAGE_BYTES (48 * 1024 * 1024)

static void *bitmap_create(int width, int height) {
  Serial.printf("ESP.getFreeHeap() -> %d\n", ESP.getFreeHeap());
  Serial.printf("bitmap_create(%d,%d) .. ", width, height);
  if (width > 75 || height > 75) {
    Serial.print(" -> NULL\n");
    return NULL;
  }
  // void *r = malloc(width * height * BYTES_PER_PIXEL);
  void *r = calloc(width * height, BYTES_PER_PIXEL);
  Serial.printf(" -> %08X\n", r); delay(250);
  return r;
}

static void bitmap_set_opaque(void *bitmap, bool opaque) {
  Serial.printf("bitmap_set_opaque()\n");
}

static bool bitmap_test_opaque(void *bitmap)
{
  Serial.printf("bitmap_test_opaque()\n");
  return false;
}


static unsigned char *bitmap_get_buffer(void *bitmap)
{
  Serial.printf("bitmap_get_buffer()\n");
  return (unsigned char *)bitmap;
}


static void bitmap_destroy(void *bitmap)
{
  Serial.printf("bitmap_destroy()\n");
  free(bitmap);
}

static void bitmap_modified(void *bitmap)
{
  Serial.printf("bitmap_modified()\n");
  return;
}

const char *LoadGif::pattern = "GET %s HTTP/1.1\r\n"
	"User-Agent: ESP8266-ESP32 Alarm Console/1.0\r\n"
	"Accept: */*\r\n"
	"Host: %s \r\n"
	"Connection: close\r\n"
	"\r\n";

// Fetch a gif over the internet
// FIX ME this needs to handle timeouts
// FIX ME this needs to handle slow connections better
char *LoadGif::loadGif(const char *url, size_t *data_size) {
  char *query, *host;

  char *proto = strstr(url, "://");
  if (proto) {
    proto += 3;
    char *s = strchr(proto, '/');
    if (s != 0) {
      host = (char *)malloc(s - proto + 2);
      for (int i=0; proto[i] != '/'; i++) {
        host[i] = proto[i];
	host[i+1] = 0;
      }
    }
  }
  if (host == 0)
    host = strdup(url);	// FIX ME

  query = (char *)malloc(strlen(url) + strlen(host) + strlen(pattern));
  sprintf(query, pattern, url, host);

  if (http == 0)
    http = new WiFiClient();

  Serial.printf("Querying %s for %s .. ", host, url);

  if (! http->connect(host, 80)) {	// Not connected
    Serial.printf("Could not connect to %s\n", host);
    return 0;
  }
  http->print(query);
  http->flush();
  // Serial.println("Query sent");

  // Postpone allocating buffer so we allocate just what's passed to us.
  // buf = (char *)malloc(buflen);
  buflen = std_buflen;

  boolean skip = true;
  int rl = 0;
  while (http->connected() || http->available()) {
    if (skip) {
      String line = http->readStringUntil('\n');
      // Serial.printf("Read headers : %s\n", line.c_str());
      const char *l = line.c_str();
      if (strncmp(l, "Content-Length:", 15) == 0) {
        buflen = atoi(l + 15) + 1;
	// Serial.printf("Headers : length is %d\n", buflen-1);
      }
      if (line.length() <= 1)
        skip = false;
    } else {
      // Allocate here so we can take the info from the header line
      buf = (char *)malloc(buflen);

      int nb = http->read((uint8_t *)&buf[rl], buflen - rl);
      if (nb > 0) {
        rl += nb;
	if (rl > buflen)
	  rl = buflen;
      } else if (nb <= 0) {
        Serial.println("Read error");
	free(query);
	free(host);

	http->stop();
	delete http;
	http = 0;

	if (data_size)
	  *data_size = 0;
	return 0;
      }
    }
    delay(100);
  }
  // Serial.println("Reply ok");

  http->stop();
  delete http;
  http = 0;

  free(query);
  free(host);

  if (data_size)
    *data_size = rl;

  Serial.printf("ok (%d bytes).\n", rl);
  return buf;
}
