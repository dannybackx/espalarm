/*
 * Helper class to make coordinates more uniform across operations
 * This is a layer above Bodmer's TFT_eSPI.
 *
 * adding functionality to turn on/off the LED
 * adding screens
 *
 * Copyright (c) 2017 by Danny Backx
 */
#include <TFT_eSPI.h>
#include <SPI.h>

#ifndef	_OLED_H_
#define	_OLED_H_

struct OledScreen {
  char	*name;		// useful ?
  int	number;

  int	nbuttons;
  char	*buttonText;
  void	(*buttonHandler[])(void);

  void	(*draw)(void);
};

class Oled : public TFT_eSPI {
  public:
    Oled(int16_t _W = TFT_WIDTH, int16_t _H = TFT_HEIGHT);
    void init(void);
    void begin(void);
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void setRotation(uint8_t r);
    void drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);
    void drawCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, uint32_t color);
    void fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color);
    void fillCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, int32_t delta, uint32_t color);
    void fillScreen(uint32_t color);
    uint8_t getTouchRaw(uint16_t *x, uint16_t *y);
    uint16_t getTouchRawZ(void);
    uint8_t getTouch(uint16_t *x, uint16_t *y);

    void setLED(int tm);

    int addScreen(OledScreen screen);
    void showScreen(int);
    boolean isScreenVisible(int);

  private:
    int verbose;
    int	led_state;
    int led_time;	// -1 is permamently on, 0 is off, another value is timeout

    int screen_nr;	// screen number currently shown
    int nscreens;	// Number of screens active
    OledScreen *screens;
};

#endif
