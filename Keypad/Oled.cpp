/*
 * Helper class to make coordinates more uniform across operations
 *
 * Copyright (c) 2017 by Danny Backx
 */
#include <Oled.h>

Oled::Oled(int16_t _W, int16_t _H) {
  TFT_eSPI(_W, _H);
}

void Oled::init(void) {
  TFT_eSPI::init();
}

void Oled::begin(void) {
  TFT_eSPI::begin();
}

void Oled::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
  TFT_eSPI::drawRect(x, y, w, h, color);
}

void Oled::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
  TFT_eSPI::fillRect(x, y, w, h, color);
}

void Oled::setRotation(uint8_t r) {
  TFT_eSPI::setRotation(r);
}

void Oled::drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
  TFT_eSPI::drawCircle(x0, y0, r, color);
}

void Oled::fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
  TFT_eSPI::fillCircle(x0, y0, r, color);
}

void Oled::fillScreen(uint32_t color) {
  TFT_eSPI::fillScreen(color);
}

uint8_t Oled::getTouchRaw(uint16_t *x, uint16_t *y) {
  return TFT_eSPI::getTouchRaw(x, y);
}

uint16_t Oled::getTouchRawZ(void) {
  return TFT_eSPI::getTouchRawZ();
}

uint8_t Oled::getTouch(uint16_t *x, uint16_t *y) {
  return TFT_eSPI::getTouch(x, y);
}
