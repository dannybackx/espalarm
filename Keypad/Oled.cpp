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

void Oled::showScreen(int ix) {
  curr_screen = ix;
  if (verbose) Serial.printf("ShowScreen(%d) %s\n", ix, screens[ix].name.c_str());

  // fillScreen(TFT_BLACK);			// Clear the screen
  if (screens[curr_screen].draw != 0) {
    screens[curr_screen].draw(&screens[curr_screen]);
  }
}

boolean Oled::isScreenVisible(int ix) {
  return (curr_screen == ix);
}
