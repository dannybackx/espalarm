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
    void loop(void);

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

  private:
    int verbose;
    int	led_state;
    int led_time;	// -1 is permamently on, 0 is off, another value is timeout

    int curr_screen;	// screen number currently shown

    std::vector<OledScreen> screens;

    void showScreenButtons(int);
    OledScreen *current;
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
#endif
