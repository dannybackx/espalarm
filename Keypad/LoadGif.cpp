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

#include <Oled.h>

static void *bitmap_create(int width, int height);
static void bitmap_destroy(void *bitmap);
static void bitmap_set_opaque(void *bitmap, bool opaque);
static bool bitmap_test_opaque(void *bitmap);
static unsigned char *bitmap_get_buffer(void *bitmap);
static void bitmap_modified(void *bitmap);

LoadGif::LoadGif() {
  pixels = 0;
  buf = 0;

  bitmap_callbacks.bitmap_create = bitmap_create;
  bitmap_callbacks.bitmap_destroy = bitmap_destroy;
  bitmap_callbacks.bitmap_get_buffer = bitmap_get_buffer;
  bitmap_callbacks.bitmap_set_opaque = bitmap_set_opaque;
  bitmap_callbacks.bitmap_test_opaque = bitmap_test_opaque;
  bitmap_callbacks.bitmap_modified = bitmap_modified;
}

LoadGif::~LoadGif() {
  if (buf)
    free(buf);
  buf = 0;
}

void LoadGif::loadGif(const char *url) {
				Serial.printf("LoadGif(%s)\n", url);

  size_t giflen;
  gif_result code;
  unsigned char *gifdata;

				// Serial.printf("Before GIF : heap %d\n", ESP.getFreeHeap());
  gif_create(&gif, &bitmap_callbacks);
				// Serial.printf("Before download : heap %d\n", ESP.getFreeHeap());

  // Download it
  gifdata = loadGif(url, &giflen);
				// Print first bytes, should read "GIF89a"
				// for (int i=0; i<8; i++)
				//   Serial.printf("%02x %c ", gifdata[i], gifdata[i] ? gifdata[i] : '.');
				// Serial.println();
				// Serial.printf("After download : heap %d\n", ESP.getFreeHeap());
  if (gifdata == 0)
    return;

				// Serial.print("Begin decoding .. ");
  /* begin decoding */
  do {
    code = gif_initialise(&gif, giflen, gifdata);
    if (code != GIF_OK && code != GIF_WORKING) {
      // warning("gif_initialise", code);

      // FIXME
      free(buf);
      buf = 0;

      return;
    }
  } while (code != GIF_OK);

  // Converted picture
  pic = Decode2(&gif);
  picw = gif.width;
  pich = gif.height;

  /* clean up */
  gif_finalise(&gif);

				// Serial.printf("LoadGif conversion done (%d x %d)\n", picw, pich);

  oled.drawIcon(pic, 100, 100, picw, pich);

				// Serial.println("done");
				// Serial.printf("After GIF decode : heap %d\n", ESP.getFreeHeap());

  free(buf);		// Frees the buffer allocated in loadGif()
  buf = 0;
}

#define BYTES_PER_PIXEL 2
#define MAX_IMAGE_BYTES (48 * 1024 * 1024)

static void *bitmap_create(int width, int height) {
				// Serial.printf("ESP.getFreeHeap() -> %d\n", ESP.getFreeHeap());
				// Serial.printf("bitmap_create(%d,%d) .. ", width, height);
  if (width > 75 || height > 75) {
				// Serial.print(" -> NULL\n");
    return NULL;
  }

  void *r = calloc(width * height, BYTES_PER_PIXEL);
				// Serial.printf(" -> %08X\n", r); delay(250);
  return r;
}

static void bitmap_set_opaque(void *bitmap, bool opaque) {
  // Serial.printf("bitmap_set_opaque()\n");
}

static bool bitmap_test_opaque(void *bitmap)
{
  // Serial.printf("bitmap_test_opaque()\n");
  return false;
}


static unsigned char *bitmap_get_buffer(void *bitmap)
{
  // Serial.printf("bitmap_get_buffer()\n");
  return (unsigned char *)bitmap;
}


static void bitmap_destroy(void *bitmap)
{
  // Serial.printf("bitmap_destroy()\n");
  free(bitmap);
}

static void bitmap_modified(void *bitmap)
{
  // Serial.printf("bitmap_modified()\n");
  return;
}

const char *LoadGif::pattern = "GET %s HTTP/1.1\r\n"
	"User-Agent: ESP8266-ESP32 Alarm Console/1.0\r\n"
	"Accept: */*\r\n"
	"Host: %s\r\n"
	"Connection: close\r\n"
	"\r\n";

// Fetch a gif over the internet
// FIX ME this needs to handle timeouts
// FIX ME this needs to handle slow connections better
// Return points to a buffer which the caller needs to free.
unsigned char *LoadGif::loadGif(const char *url, size_t *data_size) {
  char		*query, *host;
  WiFiClient	http;

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
  if (host == 0) {
    // FIX ME maybe just give up here and return NULL
    host = strdup(url);
    Serial.printf("Could not get host from %s\n", url);
  }

  query = (char *)malloc(strlen(url) + strlen(host) + strlen(pattern) + 8);
  sprintf(query, pattern, url, host);
				// Serial.printf("Query gif from %s .. ", host);

  if (! http.connect(host, 80)) {	// Not connected
    Serial.printf("Could not connect to %s\n", host);
    return 0;
  }
  http.print(query);
  http.flush();

  buflen = std_buflen;		// Postpone allocating buffer, base buffer length on header info
  boolean skip = true;
  int rl = 0;
  while (http.connected() || http.available()) {
    if (skip) {			// Skip headers, but read Content-Length
      String line = http.readStringUntil('\n');
      const char *l = line.c_str();
      if (strncmp(l, "Content-Length:", 15) == 0) {
        buflen = atoi(l + 15) + 1;
      }
      if (line.length() <= 1)	// Headers end after an empty line
        skip = false;
    } else {
      // Allocate here, based on the header info
      if (buf)		// Prevent leak
        free(buf);
      buf = (unsigned char *)malloc(buflen);

      int nb = http.read((uint8_t *)&buf[rl], buflen - rl);
      if (nb > 0) {
        rl += nb;
	if (rl > buflen)
	  rl = buflen;
      } else if (nb <= 0) {
        Serial.println("Read error");
	free(query);
	free(host);
	if (buf) {
	  free(buf);
	  buf = 0;
	}

	http.stop();

	if (data_size)
	  *data_size = 0;
	return 0;
      }
    }
    delay(100);
  }
				// Serial.printf("reply ok, downloaded %d bytes\n", rl);

  http.stop();

  free(query);
  free(host);

  if (data_size)
    *data_size = rl;

  return buf;
}

extern unsigned char mostlycloudy_gif[];
extern int mostlycloudy_len;

void LoadGif::loop(time_t) {
  // if (mostlycloudy_len)
  //   TestIt(mostlycloudy_gif, mostlycloudy_len);
  // mostlycloudy_len = 0;
}

uint16_t *LoadGif::Decode2(gif_animation *gif) {
  gif_result code;

  uint16_t *outbuf = (uint16_t *)malloc(gif->width * gif->height * 2 + 8);
				// Serial.printf("Decode2 : sz %d (%d x %d)\n", gif->width * gif->height * 2 + 8, gif->width, gif->height);

  /* decode the frames */
  for (int i = 0; i != gif->frame_count; i++) {
    int row, col, k;
    unsigned char *image;

    code = gif_decode_frame(gif, i);
    if (code != GIF_OK)
      ; // warning("gif_decode_frame", code);

    image = (unsigned char *) gif->frame_image;
    for (row = 0, k = 0; row != gif->height; row++) {
      for (col = 0; col != gif->width; col++) {
	size_t z = (row * gif->width + col) * 4;
	// 5 + 6 + 5 bits coded
				// Serial.printf("%04x ", (uint16_t)(image[z] << 11 | image[z+1] << 6 | image[z+2]));
	// k++ is equivalent to (row * gif->width + col)
	outbuf[k++] = image[z] << 11 | image[z+1] << 6 | image[z+2];
      }
    }
  }
  return outbuf;
}

void LoadGif::TestIt(unsigned char data[], int len) {
  Serial.printf("TestIt(_, %d) .. ", len);

  gif_bitmap_callback_vt bitmap_callbacks = {
    bitmap_create,
    bitmap_destroy,
    bitmap_get_buffer,
    bitmap_set_opaque,
    bitmap_test_opaque,
    bitmap_modified
  };
  gif_animation gif;
  size_t size;
  gif_result code;
  unsigned char *gifdata;

  /* create our gif animation */
  gif_create(&gif, &bitmap_callbacks);

  /* load file into memory */
  gifdata = data;
  size = len;

  /* begin decoding */
  do {
    code = gif_initialise(&gif, size, gifdata);
    if (code != GIF_OK && code != GIF_WORKING) {
      // warning("gif_initialise", code);
      return;
    }
  } while (code != GIF_OK);

  // Converted picture
  pic = Decode2(&gif);
  picw = gif.width;
  pich = gif.height;

  /* clean up */
  gif_finalise(&gif);

  Serial.printf("TestIt done (%d x %d)\n", picw, pich);

  oled.drawIcon(pic, 100, 200, picw, pich);
}

/*
 * FIX ME
 * This is meant to be a way to read a rectangle from the screen, but incomplete
 * implementation, and untested.
 * See examples/320 x 240/TFT_Screen_Capture/screenServer.ino
 */
void LoadGif::ReadScreen(uint16_t *data, int x, int y, int width, int height) {
  oled.readRect(x, y, width * height, 1, data);
}
