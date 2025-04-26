#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <SD_MMC.h>

#define MY_SD SD_MMC

#define LOG_LEVEL_VERBOSE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2

#define LOG_LEVEL LOG_LEVEL_DEBUG

#define RGB888_TO_RGB565(r, g, b) \
    ((((r >> 3) & 0x1F) << 11) | \
     (((g >> 2) & 0x3F) << 5)  | \
     ((b >> 3) & 0x1F)) 

// #define WQX_COLOR 0x0000FF

// #define WQX_COLOR_RGB565_BG RGB888_TO_RGB565(0x84, 0xb2, 0x84)
#define WQX_COLOR_RGB565_BG RGB888_TO_RGB565(0x95, 0xb2, 0x55)
#define WQX_COLOR_RGB565_FG RGB888_TO_RGB565(0x00, 0x00, 0x00)
// WQX background color #84b284 or #95b34f or #95b255


#endif
