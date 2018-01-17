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
  pixels = 0;

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

  size_t giflen;
  gif_result code;
  char *gifdata;

				// Serial.printf("Before GIF : heap %d\n", ESP.getFreeHeap());
  gif_create(&gif, &bitmap_callbacks);
				// Serial.printf("Before download : heap %d\n", ESP.getFreeHeap());

  // Download it
  gifdata = loadGif(url, &giflen);
				// Print first bytes, should read "GIF89a"
				for (int i=0; i<8; i++)
				  Serial.printf("%02x %c ", gifdata[i], gifdata[i] ? gifdata[i] : '.');
				Serial.println();
				// Serial.printf("After download : heap %d\n", ESP.getFreeHeap());
  if (gifdata == 0)
    return;

  Serial.print("Begin decoding .. "); delay(250);
  do {
    code = gif_initialise(&gif, giflen, (unsigned char *)gifdata);
    if (code != GIF_OK && code != GIF_WORKING) {
      Serial.printf("LoadGif(%s) failed : %d\n", url, code);
      return;
    }
    Serial.printf("LoadGif(%s) gif_initialise -> %d\n", url, code);
  } while (code != GIF_OK);

#if 0
  Serial.print(" (phase 2 disabled)");
#else
  Serial.printf(" --> phase 2 .. (has %d frames) ", gif.frame_count);

  // Handle only one frame
  unsigned char *image;

  code = gif_decode_frame(&gif, 0);	// Only first frame
  Serial.printf("After gif_decode_frame()\n");
  if (code != GIF_OK) {
    Serial.printf("gif error, code %d\n", code);
    // FIX ME
  } else {
    image = (unsigned char *)gif.frame_image;
    pixels = (uint16_t *)malloc(gif.height * gif.width * 2);

    for (int row=0; row != gif.height; row++)
      for (int col=0; col != gif.width; col++) {
        Serial.printf(".. %d %d\n", row, col);
	size_t z = (row * gif.width + col) * 4;
	uint16_t x = image[z] << 11 + image[z+1] << 5 + image[z+2];
	pixels[row * gif.width + col] = x;
      }
  }

#endif
  Serial.println("done");
  // Serial.printf("After GIF decode : heap %d\n", ESP.getFreeHeap());

  gif_finalise(&gif);
  free(gifdata);		// Frees the buffer allocated in loadGif()
}

#define BYTES_PER_PIXEL 2
#define MAX_IMAGE_BYTES (48 * 1024 * 1024)

static void *bitmap_create(int width, int height) {
  // Serial.printf("ESP.getFreeHeap() -> %d\n", ESP.getFreeHeap());
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
	"Host: %s\r\n"
	"Connection: close\r\n"
	"\r\n";

// Fetch a gif over the internet
// FIX ME this needs to handle timeouts
// FIX ME this needs to handle slow connections better
// Return points to a buffer which the caller needs to free.
char *LoadGif::loadGif(const char *url, size_t *data_size) {
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
  Serial.printf("Query gif from %s .. ", host);

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
      buf = (char *)malloc(buflen);

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
  Serial.printf("reply ok, downloaded %d bytes\n", rl);

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
  if (mostlycloudy_len)
    TestIt(mostlycloudy_gif, mostlycloudy_len);
  mostlycloudy_len = 0;
}

#if 0
void LoadGif::TestIt(unsigned char data[], int len) {
  Serial.printf("TestIt(_, %d)\n", len);

  size_t giflen;
  gif_result code;
  char *gifdata;

				// Serial.printf("Before GIF : heap %d\n", ESP.getFreeHeap());
  gif_create(&gif, &bitmap_callbacks);
				// Serial.printf("Before download : heap %d\n", ESP.getFreeHeap());

  // Download it
  // gifdata = loadGif(url, &giflen);
  gifdata = (char *)data;
  giflen = len;
				// Print first bytes, should read "GIF89a"
				for (int i=0; i<8; i++)
				  Serial.printf("%02x %c ", gifdata[i], gifdata[i] ? gifdata[i] : '.');
				Serial.println();
				// Serial.printf("After download : heap %d\n", ESP.getFreeHeap());
  if (gifdata == 0)
    return;

  Serial.print("Begin decoding .. "); delay(250);
  do {
    code = gif_initialise(&gif, giflen, (unsigned char *)gifdata);
    if (code != GIF_OK && code != GIF_WORKING) {
      Serial.printf("LoadGif(%s) failed : %d\n", "URL", code);
      return;
    }
    Serial.printf("LoadGif(%s) gif_initialise -> %d\n", "URL", code);
  } while (code != GIF_OK);

  Serial.printf(" --> phase 2 .. (has %d frames) ", gif.frame_count);

  // Handle only one frame
  unsigned char *image;

  code = gif_decode_frame(&gif, 0);	// Only first frame
  Serial.printf("After gif_decode_frame()\n");
  if (code != GIF_OK) {
    Serial.printf("gif error, code %d\n", code);
    // FIX ME
  } else {
    image = (unsigned char *)gif.frame_image;
    pixels = (uint16_t *)malloc(gif.height * gif.width * 2);

    for (int row=0; row != gif.height; row++)
      for (int col=0; col != gif.width; col++) {
        Serial.printf(".. %d %d\n", row, col);
	size_t z = (row * gif.width + col) * 4;
	uint16_t x = image[z] << 11 + image[z+1] << 5 + image[z+2];
	pixels[row * gif.width + col] = x;
      }
  }

  Serial.println("done");
  // Serial.printf("After GIF decode : heap %d\n", ESP.getFreeHeap());

  gif_finalise(&gif);
  free(gifdata);		// Frees the buffer allocated in loadGif()
}
#else
static void write_ppm(FILE* fh, const char *name, gif_animation *gif, bool no_write) {
        unsigned int i;
        gif_result code;

        Serial.printf("P3\n");
        Serial.printf("# %s\n", name);
        Serial.printf("# width                %u \n", gif->width);
        Serial.printf("# height               %u \n", gif->height);
        Serial.printf("# frame_count          %u \n", gif->frame_count);
        Serial.printf("# frame_count_partial  %u \n", gif->frame_count_partial);
        Serial.printf("# loop_count           %u \n", gif->loop_count);
        Serial.printf("%u %u 256\n", gif->width, gif->height * gif->frame_count);

        /* decode the frames */
        for (i = 0; i != gif->frame_count; i++) {
                unsigned int row, col;
                unsigned char *image;

                code = gif_decode_frame(gif, i);
                if (code != GIF_OK)
                        ; // warning("gif_decode_frame", code);

                if (!no_write) {
                        Serial.printf("# frame %u:\n", i);

                        image = (unsigned char *) gif->frame_image;
                        for (row = 0; row != gif->height; row++) {
                                for (col = 0; col != gif->width; col++) {
                                        size_t z = (row * gif->width + col) * 4;

                                        Serial.printf("%u %u %u ",
                                          (unsigned char) image[z],
                                          (unsigned char) image[z + 1],
                                          (unsigned char) image[z + 2]);
                                }
                                Serial.printf("\n");
                        }
                }
        }
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
        FILE *outf = stdout;
        bool no_write = false;

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

        write_ppm(outf, "mostlycloudy.gif", &gif, no_write);
        /* clean up */
        gif_finalise(&gif);
Serial.print("done\n");
        return;


}
#endif
