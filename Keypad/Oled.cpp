/*
 * Helper class to make coordinates more uniform across operations
 * For both TFT and Touch, the 0,0 coordinates are in the lower left corner,
 * assuming the VCC pin is on the lower left and T_IRQ is on the lower right.
 *
 * This is a layer above Bodmer's TFT_eSPI.
 *
 * adding functionality to turn on/off the LED
 * adding screens
 *
 * Copyright (c) 2017 by Danny Backx
 */
#include <Oled.h>

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
#if 0
  // Unchanged
  if (verbose) Serial.printf("drawRect(%d,%d,%d,%d, %08x)\n", x, y, w, h, color);
  TFT_eSPI::drawRect(x, y, w, h, color);
#else
  if (verbose) Serial.printf("drawRect(%d,%d,%d,%d, %08x)\n", x, 320 - y - h, w, h, color);
  TFT_eSPI::drawRect(x, 320 - y - h, w, h, color);
#endif
}

void Oled::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
#if 0
  // Unchanged
  if (verbose) Serial.printf("fillRect(%d,%d,%d,%d, %08x)\n", x, y, w, h, color);
  TFT_eSPI::fillRect(x, y, w, h, color);
#else
  if (verbose) Serial.printf("fillRect(%d,%d,%d,%d, %08x)\n", x, 320 - y - h, w, h, color);
  TFT_eSPI::fillRect(x, 320 - y - h, w, h, color);
#endif
}

void Oled::setRotation(uint8_t r) {
  TFT_eSPI::setRotation(r);
}

void Oled::drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
  if (verbose) Serial.printf("drawCircle(%d,%d,%d,%d, %08x)\n", x0, 320 - y0, r, color);
  TFT_eSPI::drawCircle(x0, 320 - y0, r, color);
}

void Oled::fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
  if (verbose) Serial.printf("fillCircle(%d,%d,%d,%d, %08x)\n", x0, 320 - y0, r, color);
  TFT_eSPI::fillCircle(x0, 320 - y0, r, color);
}

void Oled::fillScreen(uint32_t color) {
  TFT_eSPI::fillScreen(color);
}

uint8_t Oled::getTouchRaw(uint16_t *x, uint16_t *y) {
  uint16_t a, b;
  uint8_t r = TFT_eSPI::getTouchRaw(&a, &b);

  *x = 240 - 240 * (a - 150) / 4000;
  *y = 320 - 320 * (b - 150) / 4000;

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

#define LABEL1_FONT &FreeSansOblique12pt7b // Key label font 1
#define LABEL2_FONT &FreeSansBold12pt7b    // Key label font 2

// Keypad start position, key sizes and spacing
#define KEY_X		40	// Centre of key
#define KEY_Y		20	// 96
#define KEY_W		62	// Width and height
#define KEY_H		30
#define KEY_SPACING_X	18 // X and Y gap
#define KEY_SPACING_Y	20
#define KEY_TEXTSIZE	1   // Font size multiplier

void Oled::showScreenButtons(int ix) {
  uint8_t row = 0;
  if (current->key == 0)
    current->key = (TFT_eSPI_Button **)calloc(sizeof(TFT_eSPI_Button *), current->nbuttons+1);
  for (uint8_t col = 0; col < current->nbuttons; col++) {
    uint8_t b = col + row * 3;

    if (current->key[b] == nullptr) {
      TFT_eSPI_Button *btn = new TFT_eSPI_Button();
      current->key[b] = btn;
    }

    if (b < 3)
      setFreeFont(LABEL1_FONT);
    else
      setFreeFont(LABEL2_FONT);

    current->key[b]->initButton(this,
      KEY_X + col * (KEY_W + KEY_SPACING_X),
      KEY_Y + row * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
      KEY_W, KEY_H, TFT_WHITE, TFT_RED /* keyColor[b] */, TFT_WHITE,
      (char *)current->buttonText[b].c_str(), KEY_TEXTSIZE);
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
