/***************************************
   Start of Canvas (framebuffer)
 **************************************/

// 1.47" IPS round corner LCD 172x320
Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 1 /* rotation */, true /* IPS */, 172 /* width */, 320 /* height */, 34 /* col offset 1 */, 0 /* row offset 1 */, 34 /* col offset 2 */, 0 /* row offset 2 */);
