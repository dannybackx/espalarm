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

#ifndef	_LOADGIF_H_
#define	_LOADGIF_H_

#include "libnsgif.h"

class LoadGif {
public:
  LoadGif();
  ~LoadGif();
  void loop(time_t);

  void loadGif(const char *);

private:
  static const char	*pattern;

  int			buflen;
  const int		std_buflen = 2000;
  char			*buf;

  uint16_t		*pixels;
  gif_animation		gif;

  char *loadGif(const char *url, size_t *data_size);
  gif_bitmap_callback_vt bitmap_callbacks;
  uint16_t *Decode2(const char *name, gif_animation *gif);

  void TestIt(unsigned char [], int);

  uint16_t *pic, picw, pich;
};

extern LoadGif *gif;

#endif	/* _LOADGIF_H_ */
