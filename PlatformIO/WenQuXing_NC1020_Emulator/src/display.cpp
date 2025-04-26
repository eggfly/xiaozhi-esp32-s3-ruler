
#include "display.h"


Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK /* SCK */, TFT_MOSI /* MOSI */, GFX_NOT_DEFINED /* MISO */, HSPI /* spi_num */);

// 1.47" IPS round corner LCD 172x320
Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 0 /* rotation */, true /* IPS */, SCREEN_WIDTH /* width */, SCREEN_HEIGHT /* height */, 34 /* col offset 1 */, 0 /* row offset 1 */, 34 /* col offset 2 */, 0 /* row offset 2 */);

// Arduino_GFX & canvas = *gfx;
// Arduino_Canvas(int16_t w, int16_t h, Arduino_G *output, int16_t output_x = 0, int16_t output_y = 0, uint8_t rotation = 0);

Arduino_GFX * canvas_ptr = new Arduino_Canvas(SCREEN_WIDTH, SCREEN_HEIGHT, gfx, 0, 0, 1 /* rotation */);

Arduino_GFX & canvas = *canvas_ptr;

void initScreen() {
  Serial.println("Arduino_GFX!");

  // Init Display
  if (!gfx->begin())
    // if (!gfx->begin(80000000)) /* specify data bus speed */
  {
    Serial.println("gfx->begin() failed!");
  }

  if (!canvas.begin()) {
    Serial.println("canvas.begin() failed!");
  }

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, LOW);
#endif

  // gfx->fillScreen(0x00FF0000);

}
