/*
 * Helper class to make coordinates more uniform across operations
 *
 * For both TFT and Touch, the 0,0 coordinates are in the lower left corner,
 * assuming the VCC pin is on the lower left and T_IRQ is on the lower right.
 *
 * This is a layer above Bodmer's TFT_eSPI,
 * which adds functionality to turn on/off the LED,
 * screens,
 * and additional functionality to the {TFT_/Oled}Button function.
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

#include <Oled.h>
#include <BackLight.h>

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

// #define	LABEL_FONT		&FreeSans12pt7b
#define	LABEL_FONT		&FreeSans9pt7b

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
void Oled::loop(time_t nowts) {
  if (getTouchRawZ() > 500) {
    uint16_t tx, ty;

    backlight->touched(nowts);

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
#if 0
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

/*
 * Note the Bodmer version of drawIcon is for FLASH memory. No such thing here.
 * See TFT_eSPI/examples/320 x 240/TFT_Flash_Bitmap , moved from example to class method here.
 */
void Oled::drawIcon(const uint16_t *icon, int16_t x, int16_t y, uint16_t width, uint16_t height) {
#if 1
  // Use buffering to increase transfer speed.
  // We don't need this speed, but the examples are coded this way so what the hell.

  uint16_t pixbuf[OLED_BS];

  setWindow(x, y, x + width - 1, y + height - 1);

  // How many whole buffers to send ?
  uint16_t nb = ((uint16_t)height * width) / OLED_BS;

  // Fill and send that many buffers to TFT
  for (int i = 0; i < nb; i++) {
    for (int j = 0; j < OLED_BS; j++) {
      pixbuf[j] = icon[i * OLED_BS + j];
    }
    pushColors(pixbuf, OLED_BS);
  }

  // How many pixels not yet sent ?
  uint16_t np = ((uint16_t)height * width) % OLED_BS;

  // Send any partial buffer left over
  if (np) {
    for (int i = 0; i < np; i++)
      pixbuf[i] = icon[nb * OLED_BS + i];

      pushColors(pixbuf, np);
    }
#else
  // Slow version, no buffering
  for (int i=0; i<width*height; i++)
    pushColor(icon[i]);
#endif
}

void Oled::fontSize(int i) {
  switch (i) {
  default:	setFreeFont(&FreeSans9pt7b); break;
  case 2:	setFreeFont(&FreeSans12pt7b); break;
  case 3:	setFreeFont(&FreeSans18pt7b); break;
  case 4:	setFreeFont(&FreeSans24pt7b); break;
  }
}

void OledButton::setFillColor(uint16_t newfc) {
  if (newfc != _fillcolor) {
    _fillcolor = newfc;
    drawButton(false);
  }
}

void OledButton::setText(const char *txt) {
  strncpy(_label, txt, sizeof(_label)-1);
  _label[sizeof(_label)-1] = 0;
}
