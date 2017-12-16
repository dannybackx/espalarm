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

#include <Oled.h>

const int ctrx = 1;	// Translate coordinates ?

Oled::Oled(int16_t _W, int16_t _H) {
  TFT_eSPI(_W, _H);
  verbose = 0;
}

void Oled::init(void) {
  TFT_eSPI::init();
  screens = std::vector<OledScreen>(0);
}

void Oled::begin(void) {
  TFT_eSPI::begin();
}

void Oled::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
  if (verbose) Serial.printf("drawRect(%d,%d,%d,%d, %08x)\n", x, 320 - y - h, w, h, color);
  if (ctrx)
    TFT_eSPI::drawRect(x, 320 - y - h, w, h, color);
  else
    TFT_eSPI::drawRect(x, y, w, h, color);
}

void Oled::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
  if (verbose) Serial.printf("fillRect(%d,%d,%d,%d, %08x)\n", x, 320 - y - h, w, h, color);
  if (ctrx)
    TFT_eSPI::fillRect(x, 320 - y - h, w, h, color);
  else
    TFT_eSPI::fillRect(x, y, w, h, color);
}

void Oled::setRotation(uint8_t r) {
  TFT_eSPI::setRotation(r);
}

void Oled::drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
  if (verbose) Serial.printf("drawCircle(%d,%d,%d,%d, %08x)\n", x0, 320 - y0, r, color);
  if (ctrx)
    TFT_eSPI::drawCircle(x0, 320 - y0, r, color);
  else
    TFT_eSPI::drawCircle(x0, y0, r, color);
}

void Oled::fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
  if (verbose) Serial.printf("fillCircle(%d,%d,%d,%d, %08x)\n", x0, 320 - y0, r, color);
  if (ctrx)
    TFT_eSPI::fillCircle(x0, 320 - y0, r, color);
  else
    TFT_eSPI::fillCircle(x0, y0, r, color);
}

void Oled::fillScreen(uint32_t color) {
  TFT_eSPI::fillScreen(color);
}

uint8_t Oled::getTouchRaw(uint16_t *x, uint16_t *y) {
  uint16_t a, b;
  uint8_t r = TFT_eSPI::getTouchRaw(&a, &b);

  if (ctrx) {
    *x = 240 - 240 * (a - 300) / 3460;
    *y = 320 - 320 * (b - 300) / 3460;
  } else {
    *x = a;
    *y = b;
  }

  return r;
}

uint16_t Oled::getTouchRawZ(void) {
  return TFT_eSPI::getTouchRawZ();
}

uint8_t Oled::getTouch(uint16_t *x, uint16_t *y) {
  return TFT_eSPI::getTouch(x, y);
}

/*
 * Some functions to steer the LED (the TFT backlight)
 */
void Oled::setLED(int tm) {
}

/*
 * Screen handling
 */
int Oled::addScreen(OledScreen screen) {
  screens.push_back(screen);
  if (verbose) Serial.printf("addScreen(%s) -> pos %d\n", screen.name.c_str(), screens.size()-1);
  return screens.size()-1;
}

// FIX ME needs cleanup

#define	LABEL_FONT		&FreeSans12pt7b
// #define	LABEL_FONT		&FreeSansBold12pt7b
#define	LABEL_FONT_OBLIQUE	&FreeSansOblique12pt7b

// Keypad start position, key sizes and spacing
#define KEY_X		40	// Centre of key
#define KEY_Y		300	// 96
#define KEY_W		62	// Width and height
#define KEY_H		30
#define KEY_SPACING_X	18 // X and Y gap
#define KEY_SPACING_Y	20
#define KEY_TEXTSIZE	1   // Font size multiplier

void Oled::showScreenButtons(int ix) {
  uint8_t row = 0;
  if (current->key == 0)
    current->key = (OledButton **)calloc(sizeof(OledButton *), current->nbuttons+1);
  for (uint8_t col = 0; col < current->nbuttons; col++) {
    uint8_t b = col + row * 3;

    if (current->key[b] == nullptr) {
      OledButton *btn = new OledButton();
      current->key[b] = btn;
    }

    setFreeFont(LABEL_FONT);

    // if (verbose)
      Serial.printf("initButton(x %d, y %d, w %d, h %d) %s\n",
        KEY_X + col * (KEY_W + KEY_SPACING_X),
        KEY_Y + row * (KEY_H + KEY_SPACING_Y),
        KEY_W, KEY_H,
        (char *)current->buttonText[b].c_str());
    current->key[b]->initButton(this,
      KEY_X + col * (KEY_W + KEY_SPACING_X),	// x
      KEY_Y + row * (KEY_H + KEY_SPACING_Y),	// y
      KEY_W, KEY_H,				// w, h
      TFT_WHITE, TFT_RED, TFT_WHITE,		// outline, fill, text colours
      (char *)current->buttonText[b].c_str(), KEY_TEXTSIZE);	// text, size
    current->key[b]->drawButton();
  }
}

void Oled::showScreen(int ix) {
  curr_screen = ix;
  current = &screens[curr_screen];

  if (verbose) Serial.printf("ShowScreen(%d) %s\n", ix, screens[ix].name.c_str());

  fillScreen(TFT_BLACK);			// Clear the screen
  if (screens[curr_screen].draw != 0) {
    screens[curr_screen].draw(&screens[curr_screen]);
  }

  showScreenButtons(ix);
}

boolean Oled::isScreenVisible(int ix) {
  return (curr_screen == ix);
}

/*
 * When called periodically, this triggers button callback functions
 */
void Oled::loop(void) {
  if (getTouchRawZ() > 500) {
    uint16_t tx, ty;
    (void) getTouchRaw(&tx, &ty);

    for (int btn=0; btn<current->nbuttons; btn++)
      if (ctrx) {
      if (current->key[btn]->contains(tx, 320 - ty)) {
        current->buttonHandler[btn](current, btn);
      }
      } else {
      if (current->key[btn]->contains(tx, ty)) {
        current->buttonHandler[btn](current, btn);
      }
      }
  }
}

void OledButton::initButton(TFT_eSPI *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
	uint16_t outline, uint16_t fill, uint16_t textcolor, char *label, uint8_t textsize) {
  if (ctrx)
    TFT_eSPI_Button::initButton(gfx, x, 320 - y, w, h, outline, fill, textcolor, label, textsize);
  else
    TFT_eSPI_Button::initButton(gfx, x, y, w, h, outline, fill, textcolor, label, textsize);
}

/*
 *
 * Note the debug code relies on _x1 etc variables which are private in the superclass.
 * So this only works by editing that class definition so they're protected instead of private.
 * acer: {914} diff TFT_eSPI.h~ TFT_eSPI.h
 * 523,524c523
 * <  private:
 * <   TFT_eSPI *_gfx;
 * ---
 * >  protected:
 * 526a526,528
 * > 
 * >  private:
 * >   TFT_eSPI *_gfx;
 *
 */
boolean OledButton::contains(int16_t x, int16_t y) {
#if 1
  y = 320 - y;

  boolean r = ((x >= _x1) && (x < (_x1 + _w)) && (y >= _y1) && (y < (_y1 + _h)));
  Serial.printf("Contains ? {%d,%d} [%d .. %d, %d .. %d] -> %s\n",
    // this->name,
    x, y,
    _x1, _x1 + _w,
    _y1, _y1 + _h,
    r ? "yes" : "no");
  return ((x >= _x1) && (x < (_x1 + _w)) && (y >= _y1) && (y < (_y1 + _h)));
#else
  return TFT_eSPI_Button::contains(x, y);
#endif
}
