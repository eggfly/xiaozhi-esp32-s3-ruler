
#include "display.h"


Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK /* SCK */, TFT_MOSI /* MOSI */, GFX_NOT_DEFINED /* MISO */, HSPI /* spi_num */);

// 1.47" IPS round corner LCD 172x320
Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 1 /* rotation */, true /* IPS */, SCREEN_WIDTH /* width */, SCREEN_HEIGHT /* height */, 34 /* col offset 1 */, 0 /* row offset 1 */, 34 /* col offset 2 */, 0 /* row offset 2 */);

Arduino_GFX & canvas = *gfx;

void initScreen() {
  Serial.println("Arduino_GFX!");

  // Init Display
  if (!gfx->begin())
    // if (!gfx->begin(80000000)) /* specify data bus speed */
  {
    Serial.println("gfx->begin() failed!");
  }

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, LOW);
#endif

  gfx->fillScreen(0x00FF0000);

}
