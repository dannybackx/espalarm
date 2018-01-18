/*
 * Helper class to make coordinates more uniform across operations
 *
 * For both TFT and Touch, the 0,0 coordinates are in the lower left corner,
 * assuming the VCC pin is on the lower left and T_IRQ is on the lower right.
 *
 * This is a layer above Bodmer's TFT_eSPI,
 * which adds functionality to turn on/off the LED,
 * and screens.
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

#include <TFT_eSPI.h>
#include <SPI.h>
#include <vector>

#ifndef	_OLED_H_
#define	_OLED_H_

class OledButton;

struct OledScreen {
  String	name;		// useful ?
  int		number;

  int		nbuttons;
  String	*buttonText;
  void		(**buttonHandler)(struct OledScreen *, int);

  void		(*draw)(struct OledScreen *);
  OledButton	**key;
};

class Oled : public TFT_eSPI {
  public:
    // Generic Arduino functions
    Oled(int16_t _W = TFT_WIDTH, int16_t _H = TFT_HEIGHT);
    void init(void);
    void begin(void);
    void loop(time_t);

    // Draw stuff
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void setRotation(uint8_t r);
    void drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);
    void drawCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, uint32_t color);
    void fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);
    void fillCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, int32_t delta, uint32_t color);
    void fillScreen(uint32_t color);

    // Get touch input
    uint8_t getTouchRaw(uint16_t *x, uint16_t *y);
    uint16_t getTouchRawZ(void);
    uint8_t getTouch(uint16_t *x, uint16_t *y);

    // Control backlight
    void setLED(int tm);

    // Handle screens
    int addScreen(OledScreen screen);
    void showScreen(int);
    boolean isScreenVisible(int);

    // Draw images
    void drawIcon(const uint16_t *icon, int16_t x, int16_t y, uint16_t width, uint16_t height);

  private:
    int verbose;
    int	led_state;
    int led_time;	// -1 is permamently on, 0 is off, another value is timeout

    int curr_screen;	// screen number currently shown

    std::vector<OledScreen> screens;

    void showScreenButtons(int);
    OledScreen *current;

    static const unsigned int OLED_BS = 64;
};

/*
 * Class ported from the Adafruit_GFX library into TFT_eSPI.
 * Believed to be compatible with the original.
 */
class OledButton : public TFT_eSPI_Button {
 public:
  // "Classic" initButton() uses center & size
  void     initButton(TFT_eSPI *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
		uint16_t outline, uint16_t fill,
		uint16_t textcolor, char *label, uint8_t textsize);
  boolean contains(int16_t x, int16_t y);
};

extern Oled	oled;
#endif
