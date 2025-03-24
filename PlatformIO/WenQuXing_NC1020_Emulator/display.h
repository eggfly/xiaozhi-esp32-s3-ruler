#ifndef DISPLAY_H_
#define DISPLAY_H_


#include <Arduino_GFX_Library.h>

#define TFT_SCLK 21
#define TFT_MOSI 47

#define TFT_CS 41
#define TFT_DC 40
#define TFT_RST 45
#define GFX_BL 42

#define SCREEN_WIDTH 172
#define SCREEN_HEIGHT 320

#define DISPLAY_WIDTH SCREEN_HEIGHT
#define DISPLAY_HEIGHT SCREEN_WIDTH


extern Arduino_GFX *gfx;
extern Arduino_GFX & canvas;

void initScreen();


#endif /* DISPLAY_H_ */
