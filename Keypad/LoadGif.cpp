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
#include <WiFi.h>
#endif
#include <WiFiClient.h>

#include <Oled.h>
#include <Peers.h>
#include <Weather.h>

static void *bitmap_create(int width, int height);
static void bitmap_destroy(void *bitmap);
static void bitmap_set_opaque(void *bitmap, bool opaque);
static bool bitmap_test_opaque(void *bitmap);
static unsigned char *bitmap_get_buffer(void *bitmap);
static void bitmap_modified(void *bitmap);

LoadGif::LoadGif(Oled *oled) {
  this->oled = oled;

  pixels = 0;
  buf = 0;
  url = 0;

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
				// Serial.printf("LoadGif dtor free(%p)\n", buf);
  buf = 0;
}

void LoadGif::loadGif(const char *url) {
				// Serial.printf("LoadGif(%s)\n", url);
  if (url && this->url && (strcmp(this->url, url) == 0)) {
    // Serial.printf("Same URL as before (%s)\n", this->url);
    return;
  }
  if (this->url)
    free(this->url);
  this->url = strdup(url);

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
				// Serial.printf("LoadGif return, free buf %p\n", buf);
      buf = 0;

      return;
    }
  } while (code != GIF_OK);

				// Serial.printf("LoadGif free buf %p\n", buf);
  free(buf);		// Frees the buffer allocated in loadGif()
  buf = 0;

#if 1
  // Converted picture
  pic = Decode2(&gif);
  picw = gif.width;
  pich = gif.height;

				// Serial.printf("LoadGif gif_finalise()\n"); Serial.flush();
  /* clean up */
  gif_finalise(&gif);

				// Serial.printf("LoadGif conversion done (%d x %d)\n", picw, pich); Serial.flush();

  				// Serial.printf("LoadGif::pic %p\n", pic); Serial.flush();
  if (pic) {
    if (weather)
      weather->drawIcon(pic, picw, pich);	// Weather knows where to draw it
    // if (oled) 
    //   oled->drawIcon(pic, picx, picy, picw, pich);

    // Pass on to other nodes
    if (peers)
      peers->SendImage(pic, picw, pich);
  }
#else
  gif_finalise(&gif);
#endif

				// Serial.println("done"); Serial.flush();
				// Serial.printf("After GIF decode : heap %d\n", ESP.getFreeHeap());
}

// Note : can't change this. The library works with 4 bytes per pixel internally
#define BYTES_PER_PIXEL 4
#define MAX_IMAGE_BYTES (48 * 1024 * 1024)

static void *bitmap_create(int width, int height) {
				// Serial.printf("ESP.getFreeHeap() -> %d\n", ESP.getFreeHeap());
				// Serial.printf("bitmap_create(%d,%d) .. ", width, height);
  if (width > 75 || height > 75) {
				// Serial.print(" -> NULL\n");
    return NULL;
  }

  int nb = width * height * BYTES_PER_PIXEL;
  void *r = malloc(nb);
  // void *r = calloc(width * height, BYTES_PER_PIXEL);
				// Serial.printf(" alloc %d -> %08X\n", nb, r); Serial.flush();
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
  // Serial.printf("bitmap_destroy(%p)\n", bitmap);
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
    // host = strdup(url);
    Serial.printf("Could not get host from %s\n", url);
    return 0;
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
  while (http && (http.connected() || http.available())) {
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
      if (buf) {		// Prevent leak
        free(buf);
					// Serial.printf("Free buf %p, pending alloc\n", buf);
      }
      buf = (unsigned char *)malloc(buflen);
      					// Serial.printf("Malloc buf(%d) -> %p\n", buflen, buf);

      int nb = http.read((uint8_t *)&buf[rl], buflen - rl);
      if (nb > 0) {
        rl += nb;
	if (rl > buflen)
	  rl = buflen;
      } else if (nb <= 0) {
        Serial.println("Read error");
	free(query); query = 0;
	free(host); host = 0;
	if (buf) {
					// Serial.printf("Free buf %p\n", buf);
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

  free(query); query = 0;
  free(host); host = 0;

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

				// Serial.printf("Decode2 : sz %d (%d x %d)", gif->width * gif->height * 2, gif->width, gif->height);

  uint16_t *outbuf = (uint16_t *)malloc(gif->width * gif->height * 2 + 16);
				// Serial.printf(" -> %p\n", outbuf);

  if (outbuf == 0) {
    Serial.printf("LoadGif::Decode2 malloc failed (free heap %d)\n", ESP.getFreeHeap());
    return 0;
  }

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
  				// Serial.printf("Decode2 : done\n");
  return outbuf;
}

/*
 * FIX ME
 * This is meant to be a way to read a rectangle from the screen, but incomplete
 * implementation, and untested.
 * See examples/320 x 240/TFT_Screen_Capture/screenServer.ino
 */
void LoadGif::ReadScreen(uint16_t *data, int x, int y, int width, int height) {
  oled->readRect(x, y, width * height, 1, data);
}
