#include <Arduino.h>

#include "FS.h"
#include "SD_MMC.h"
#include "config.h"
#include "SPI.h"
#include <map>
#include <string>
#include <vector>
#include <unordered_set>
#include <Audio.h>
#include "display.h"

#include "page.h"
#include "nc1020.h"
#include "lru.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h"

#include "app.h"

portMUX_TYPE my_mutex = portMUX_INITIALIZER_UNLOCKED;

TaskHandle_t anotherCoreTaskHandle;

Audio audio;

std::vector<String> m_songFiles{};

const bool APP_DEBUG = false;

#define I2S_DOUT 7
#define I2S_BCLK 16
#define I2S_LRCK 15

// use 12 bit precission for LEDC timer
#define LEDC_TIMER_12_BIT 12

// SPIClass fspi(FSPI);
// SPIClass vspi(VSPI); // only ESP32
// SPIClass hspi(HSPI);
// FSPI is already used to send screen data.

// GFXcanvas1 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);

uint32_t chipId = 0;

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.path(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

int clk = 13;
int cmd = 14;
int d0 = 12;
int d1 = 11;
int d2 = 10;
int d3 = 9;

bool SD_Init()
{
  if (!SD_MMC.setPins(clk, cmd, d0, d1, d2, d3))
  {
    Serial.println("Pin change failed!");
    return false;
  }
  if (!SD_MMC.begin())
  {
    Serial.println("Card Mount Failed");
    return false;
  }

  uint8_t cardType = MY_SD.cardType();

  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return false;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
  }
  else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
  }
  else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
  }
  else
  {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = MY_SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  listDir(MY_SD, "/", 0);

  Serial.printf("Total space: %lluMB\n", MY_SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", MY_SD.usedBytes() / (1024 * 1024));

  delay(200);
  return true;
}

QueueHandle_t g_event_queue_handle = NULL;

// 160 * 80 / 8
uint8_t lcd_buff[1600];

// 320 * 160 / 8
uint8_t *lcd_buff_expanded;

uint8_t my_buff[0x8000];

// std::map<char, uint8_t> cardkb_to_nc1020_keymap;
// bool cardkb_to_nc1020_keymap_initialized = false;

std::map<std::string, uint8_t> nc1020_keymap = {
    {"A", 0x28},
    {"B", 0x34},
    {"C", 0x32},
    {"D", 0x2A},
    {"E", 0x22},
    {"F", 0x2B},
    {"G", 0x2C},
    {"H", 0x2D},
    {"I", 0x27},
    {"J", 0x2E},
    {"K", 0x2F},
    {"L", 0x19},
    {"M", 0x36},
    {"N", 0x35},
    {"O", 0x18},
    {"P", 0x1C},
    {"Q", 0x20},
    {"R", 0x23},
    {"S", 0x29},
    {"T", 0x24},
    {"U", 0x26},
    {"V", 0x33},
    {"W", 0x21},
    {"X", 0x31},
    {"Y", 0x25},
    {"Z", 0x30},
    {"Enter", 0x1d},
    {"Backspace", 0x3b},
    {"F1", 0x1a}, // UP
    {"F2", 0x1b}, // DOWN
    {"F3", 0x3f}, // LEFT
    {"F4", 0x1f}, // RIGHT
    {"P1", 0x0b}, // 英汉
    {"P2", 0x0c}, // 名片
    {"P3", 0x0d}, // 计算
    {"P4", 0x0a}, // 行程
    {"P5", 0x09}, // 测验
    {"P6", 0x08}, // 其他
    {"P7", 0x0e}, // 网络
};

// void nc1020_keymap_init()
// {
//   if (cardkb_to_nc1020_keymap_initialized)
//   {
//     return;
//   }
//   cardkb_to_nc1020_keymap_initialized = true;
//   cardkb_to_nc1020_keymap['1'] = 0x0B;  //"英汉"
//   cardkb_to_nc1020_keymap['2'] = 0x0C;  //"名片"
//   cardkb_to_nc1020_keymap['3'] = 0x0D;  //"计算"
//   cardkb_to_nc1020_keymap['4'] = 0x0A;  //"行程"
//   cardkb_to_nc1020_keymap['5'] = 0x09;  //"测验"
//   cardkb_to_nc1020_keymap['6'] = 0x08;  //"其他"
//   cardkb_to_nc1020_keymap['7'] = 0x0E;  //"网络"
//   cardkb_to_nc1020_keymap[0x0d] = 0x1D; // ENTER
//   cardkb_to_nc1020_keymap[0x1b] = 0x3B; // ESC

//   cardkb_to_nc1020_keymap[0xb5] = 0x1A; // UP
//   cardkb_to_nc1020_keymap[0xb6] = 0x1B; // DOWN
//   cardkb_to_nc1020_keymap[0xb4] = 0x3F; // LEFT
//   cardkb_to_nc1020_keymap[0xb7] = 0x1F; // RIGHT
//   cardkb_to_nc1020_keymap[0x99] = 0x37; // PAGE UP
//   cardkb_to_nc1020_keymap[0xa4] = 0x1E; // PAGE DOWN

//   cardkb_to_nc1020_keymap['a'] = 0x28;
//   cardkb_to_nc1020_keymap['b'] = 0x34;
//   cardkb_to_nc1020_keymap['c'] = 0x32;
//   cardkb_to_nc1020_keymap['d'] = 0x2A;
//   cardkb_to_nc1020_keymap['e'] = 0x22;
//   cardkb_to_nc1020_keymap['f'] = 0x2B;
//   cardkb_to_nc1020_keymap['g'] = 0x2C;
//   cardkb_to_nc1020_keymap['h'] = 0x2D;
//   cardkb_to_nc1020_keymap['i'] = 0x27;
//   cardkb_to_nc1020_keymap['j'] = 0x2E;
//   cardkb_to_nc1020_keymap['k'] = 0x2F;
//   cardkb_to_nc1020_keymap['l'] = 0x19;
//   cardkb_to_nc1020_keymap['m'] = 0x36;
//   cardkb_to_nc1020_keymap['n'] = 0x35;
//   cardkb_to_nc1020_keymap['o'] = 0x18;
//   cardkb_to_nc1020_keymap['p'] = 0x1C;
//   cardkb_to_nc1020_keymap['q'] = 0x20;
//   cardkb_to_nc1020_keymap['r'] = 0x23;
//   cardkb_to_nc1020_keymap['s'] = 0x29;
//   cardkb_to_nc1020_keymap['t'] = 0x24;
//   cardkb_to_nc1020_keymap['u'] = 0x26;
//   cardkb_to_nc1020_keymap['v'] = 0x33;
//   cardkb_to_nc1020_keymap['w'] = 0x21;
//   cardkb_to_nc1020_keymap['x'] = 0x31;
//   cardkb_to_nc1020_keymap['y'] = 0x25;
//   cardkb_to_nc1020_keymap['z'] = 0x30;
// }

void NC1020_Init()
{
  // nc1020_keymap_init();
  auto freq = getCpuFrequencyMhz();
  Serial.printf("cpu freq=%dMHz\n", freq);

  lcd_buff_expanded = (uint8_t *)ps_malloc(320 * 160 / 8);

  g_event_queue_handle = xQueueCreate(20, sizeof(uint8_t));
  wqx::Initialize(nullptr);
  wqx::LoadNC1020();
}

const float PROGMEM backlight_levels[] = {
    0.0,
    0.02,
    0.05,
    0.15,
    0.3,
    0.5,
    0.75,
    1.0,
};

uint8_t current_backlight_level = sizeof(backlight_levels) / sizeof(backlight_levels[0]) / 2;

void set_backlight_level()
{
}

Page *currPage;

char menu_items[][128] = {
    "* Ubuntu, with Linux 5.19 Generic",
    "  WenQuXing NC1020 Emulator",
    "  EGGFLY Music Player",
    "  Console & Keyboard Test",
    "  MicroPython Shell (1.20.0)",
    "  LVGL v8.3.8 (ESP32-S3)",
    "  Arduino-ESP32 Factory Test",
};

const uint8_t menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);

Page *menu_pages[menu_items_count];

size_t selected_menu_index = 0;

void grub_loader_menu();

class BootMenuPage : public Page
{
public:
  void onDraw()
  {
    grub_loader_menu();
  }
  bool handleKey(const char *key_str)
  {
    if (strcmp(key_str, "Up") == 0)
    {
      select_boot_item(-1);
      return true;
    }
    else if (strcmp(key_str, "Down") == 0)
    {
      select_boot_item(1);
      return true;
    }
    else if (strcmp(key_str, "Enter") == 0)
    {
      currPage = menu_pages[selected_menu_index];
      currPage->initPage();
      return true;
    }
    return false;
  }
};

int strncmpci(const char *str1, const char *str2, size_t num)
{
  int ret_code = 0;
  size_t chars_compared = 0;

  if (!str1 || !str2)
  {
    ret_code = INT_MIN;
    return ret_code;
  }

  while ((chars_compared < num) && (*str1 || *str2))
  {
    ret_code = tolower((int)(*str1)) - tolower((int)(*str2));
    if (ret_code != 0)
    {
      break;
    }
    chars_compared++;
    str1++;
    str2++;
  }
  return ret_code;
}

bool startsWithIgnoreCase(const char *pre, const char *str)
{
  return strncmpci(pre, str, strlen(pre)) == 0;
}

bool endsWithIgnoreCase(const char *base, const char *str)
{
  int blen = strlen(base);
  int slen = strlen(str);
  return (blen >= slen) && (0 == strncmpci(base + blen - slen, str, strlen(str)));
}

void populateMusicFileList(String path, size_t depth)
{
  Serial.printf("search: %s, depth=%d\n", path.c_str(), depth);
  File musicDir = MY_SD.open(path);
  bool nextFileFound;
  do
  {
    nextFileFound = false;
    File entry = musicDir.openNextFile();
    if (entry)
    {
      nextFileFound = true;
      if (entry.isDirectory())
      {
        if (depth)
        {
          populateMusicFileList(entry.path(), depth - 1);
        }
      }
      else
      {
        const bool entryIsFile = entry.size() > 4096;
        if (entryIsFile)
        {
          if (APP_DEBUG)
          {
            Serial.print(entry.path());
            Serial.print(" size=");
            Serial.println(entry.size());
          }
          if (endsWithIgnoreCase(entry.name(), ".mp3") || endsWithIgnoreCase(entry.name(), ".flac") || endsWithIgnoreCase(entry.name(), ".aac") || endsWithIgnoreCase(entry.name(), ".wav"))
          {
            m_songFiles.push_back(entry.path());
          }
        }
      }
      entry.close();
    }
  } while (nextFileFound);
}

int m_activeSongIdx{-1};

char music_title[256] = "";
char music_album[256] = "";
char music_artist[256] = "";

void autoPlayNextSong()
{
  if (m_songFiles.size() == 0)
  {
    delay(100);
    return;
  }
  if (!audio.isRunning())
  {
    Serial.println("autoPlay: playNextSong()");
    startNextSong(true);
  }
}

std::unordered_set<int> m_played_songs{};

void clearSongInfo()
{
  music_title[0] = '\0';
  music_artist[0] = '\0';
  music_album[0] = '\0';
}

bool shuffle_mode = true;

void startNextSong(bool isNextOrPrev)
{
  if (m_songFiles.size() == 0)
  {
    return;
  }
  m_played_songs.insert(m_activeSongIdx);
  if (m_played_songs.size() * 2 > m_songFiles.size())
  {
    Serial.println("re-shuffle.");
    m_played_songs.clear();
  }
  if (isNextOrPrev)
  {
    m_activeSongIdx++;
  }
  else
  {
    m_activeSongIdx--;
  }
  if (shuffle_mode)
  {
    do
    {
      m_activeSongIdx = random(m_songFiles.size());
    } while (m_played_songs.find(m_activeSongIdx) != std::end(m_played_songs));
  }
  //  if (m_activeSongIdx >= m_songFiles.size() || m_activeSongIdx < 0) {
  //    m_activeSongIdx = 0;
  //  }
  m_activeSongIdx %= m_songFiles.size();
  Serial.print("songIndex=");
  Serial.print(m_activeSongIdx);
  Serial.print(", total=");
  Serial.println(m_songFiles.size());

  if (audio.isRunning())
  {
    audio.stopSong();
  }
  clearSongInfo();
  // new_fft_pos = NEW_SPECTRUM_DISPLAY_HISTORY;
  // TODO: FIX
  // audio.connecttoSD(m_songFiles[m_activeSongIdx].c_str());

  // drawScreen();
  Serial.println(m_songFiles[m_activeSongIdx].c_str());
}

uint8_t volume = 5;

void volumeUp()
{
  if (volume < 21)
  {
    volume++;
    audio.setVolume(volume); // 0...21
  }
}

void volumeDown()
{
  if (volume > 0)
  {
    volume--;
    audio.setVolume(volume); // 0...21
  }
  else
  {
    // volume == 0;
  }
}

class MusicPlayerPage : public Page
{
public:
  void initPage()
  {
    audio.setPinout(I2S_BCLK, I2S_LRCK, I2S_DOUT);
    audio.setVolume(volume);       // 0...21
    populateMusicFileList("/", 1); // depth = 1
    Serial.print("MusicFileList length: ");
    Serial.println(m_songFiles.size());
  }
  bool handleKey(const char *key_str)
  {
    if (strcmp(key_str, "Left") == 0)
    {
      startNextSong(false);
    }
    else if (strcmp(key_str, "Right") == 0)
    {
      startNextSong(true);
    }
    else if (strcmp(key_str, "Up") == 0)
    {
      volumeUp();
    }
    else if (strcmp(key_str, "Down") == 0)
    {
      volumeDown();
    }
    return true;
  }
  void onDraw()
  {
    audio.loop();
    autoPlayNextSong();

    canvas.fillScreen(0);
    canvas.setCursor(0, 0);
    canvas.setTextColor(0xFF);
    canvas.setTextSize(3);
    if (audio.isRunning())
    {
      auto audioDuration = audio.getAudioFileDuration();
      auto currPos = audio.getAudioCurrentTime();
      auto totalPlayingTime = audio.getTotalPlayingTime();
      Serial.printf("AudioFileDuration=%d, AudioCurrentTime=%d, TotalPlayingTime=%d\n", audioDuration, currPos, totalPlayingTime);
      canvas.println(audioDuration);
      canvas.println(currPos);
      canvas.println(totalPlayingTime);
    }
    else
    {
      canvas.println("not playing.");
    }
  }
};

std::string screen_str;
class ConsolePage : public Page
{
public:
  void onDraw()
  {
    gfx_demo();
  }
  bool handleKey(const char *key_str)
  {
    screen_str += key_str;
    return true;
  }
};

class WenQuXingPage : public Page
{
public:
  void onDraw()
  {
    nc1020_loop();
  }
  bool handleKey(const char *key_str)
  {
    std::string str(key_str);
    // TODO: FIX
    // nc1020_keymap[]
    char c = '\n';
    xQueueSend(g_event_queue_handle, &c, portMAX_DELAY);
    Serial.printf("xQueueSend, key=0x%02x\n", c);
    return true;
  }
};

void setup()
{
  Serial.begin(115200);
  keyboard_setup();
  initScreen();
  // initScreenWaves();
  menu_pages[0] = new ConsolePage();
  menu_pages[1] = new WenQuXingPage();
  menu_pages[2] = new MusicPlayerPage();
  menu_pages[3] = new ConsolePage();
  menu_pages[4] = new ConsolePage();
  menu_pages[5] = new ConsolePage();
  menu_pages[6] = new ConsolePage();
  // TODO: FIX
  currPage = menu_pages[1];

  if (!SD_Init())
  {
    for (;;)
    {
    }
  }
  NC1020_Init();
  auto coreId = xPortGetCoreID();
  Serial.print("setup() running on core ");
  Serial.println(xPortGetCoreID());
  auto anotherCoreId = coreId ? 0 : 1;

  //  xTaskCreatePinnedToCore(
  //    anotherCoreTask, /* Function to implement the task */
  //    "another", /* Name of the task */
  //    10000,  /* Stack size in words */
  //    NULL,  /* Task input parameter */
  //    0,  /* Priority of the task */
  //    &anotherCoreTaskHandle,  /* Task handle. */
  //    anotherCoreId); /* Core where the task should run */
}

SPIClass *fspi = NULL;
static const int spiClk = 40000000;

void anotherCoreTask(void *parameter)
{
  // Run SPI sending screen data lopp
  Serial.print("anotherCoreTask running on core ");
  Serial.println(xPortGetCoreID());

  for (;;)
  {
    portENTER_CRITICAL(&my_mutex);
    //    memcpy(spi_master_tx_buf, canvas.getBuffer(), 19200);
    //    // Serial.println("tx/rx");
    //
    //    digitalWrite(fspi->pinSS(), LOW);
    //    fspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    //    fspi->transferBytes(spi_master_tx_buf, spi_master_rx_buf, 19200);
    //    // fspi->writeBytes(spi_master_tx_buf, 19200);
    //    fspi->endTransaction();
    //    digitalWrite(fspi->pinSS(), HIGH);
    //    portEXIT_CRITICAL(&my_mutex);
    //    spi_keyboard_handle_loop();
    delayMicroseconds(10);
    // spi_master_loop();
  }
}

void grub_loader_menu()
{
  portENTER_CRITICAL(&my_mutex);
  canvas.fillScreen(0);
  canvas.fillRect(0 + 10, 30 + 10, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 50, 0xFF);
  canvas.fillRect(0 + 10 + 3, 30 + 10 + 3, SCREEN_WIDTH - 20 - 6, SCREEN_HEIGHT - 50 - 6, 0x00);
  canvas.setCursor(0, 0);
  canvas.setTextColor(0xFF);
  canvas.setTextSize(3);
  canvas.println("----- GNU GRUB v2.02 -----");
  canvas.setTextSize(2);
  // canvas.println("   on ESP32-S3 N16R8");
  canvas.println();
  canvas.println();
  for (size_t i = 0; i < menu_items_count; i++)
  {
    if (selected_menu_index == i)
    {
      canvas.setTextColor(0x00, 0xFF);
    }
    else
    {
      canvas.setTextColor(0xFF);
    }
    canvas.println(menu_items[i]);
  }
  portEXIT_CRITICAL(&my_mutex);
  delay(100);
}

typedef struct
{
  uint8_t d[1280000];
} my_wrapped_array;

my_wrapped_array *my_arr = NULL;

void gfx_demo()
{
  portENTER_CRITICAL(&my_mutex);
  canvas.fillScreen(0);
  //  canvas.setRotation(0);
  //  canvas.writeFastHLine(0, 0, 9, 1);
  //  canvas.setRotation(1);
  //  canvas.writeFastHLine(0, 0, 9, 1);
  //  canvas.setRotation(2);
  //  canvas.writeFastHLine(0, 0, 9, 1);
  //  canvas.setRotation(3);
  //  canvas.writeFastHLine(0, 0, 9, 1);
  //  canvas.setRotation(1);
  canvas.fillRect(400, 1, 10, 10, 1);

  canvas.setCursor(0, 0);
  canvas.setTextColor(0xFF);
  canvas.setTextSize(3);
  canvas.print("--- HELLO EGGFLY WORLD ---");
  canvas.println();
  canvas.print("Console >>>");
  canvas.println();
  canvas.print(screen_str.c_str());

  portEXIT_CRITICAL(&my_mutex);
  //  Serial.println("\nThe GFXcanvas1 raw content after drawing a fast horizontal "
  //                 "line in each rotation:\n");
}

void pixelScale(uint8_t *src, uint8_t *dest)
{
  // 像素放大倍数
  const int scale = 2;

  // 源数组每行像素个数
  const int srcPitch = 160 / 8;

  // 目标数组每行像素个数
  const int destPitch = 320 / 8;

  // 遍历源数组每个像素
  for (int y = 0; y < 80; y++)
  {
    for (int x = 0; x < srcPitch; x++)
    {
      uint8_t pixel = src[y * srcPitch + x];

      // 像素扩展为2x2的小格子
      uint8_t expandedPixel = (pixel << 4) | pixel;

      // 写入目标数组
      dest[y * 2 * destPitch + 2 * x] = expandedPixel;
      dest[y * 2 * destPitch + 2 * x + 1] = expandedPixel;
      dest[(y * 2 + 1) * destPitch + 2 * x] = expandedPixel;
      dest[(y * 2 + 1) * destPitch + 2 * x + 1] = expandedPixel;
    }
  }
}
void pixelScale2(uint8_t *src, uint8_t *dest)
{
  // 像素放大倍数
  const int scale = 2;

  // 源数组每行像素个数
  const int srcPitch = 160 / 8;

  // 目标数组每行像素个数
  const int destPitch = 320 / 8;

  // 遍历源数组每个像素
  for (int y = 0; y < 80; y++)
  {
    for (int x = 0; x < srcPitch; x++)
    {
      uint8_t pixel = src[y * srcPitch + x];

      // 像素扩展为2x2的小格子
      uint8_t expandedPixel = (pixel << 4) | pixel;

      // 写入目标数组
      dest[y * 2 * destPitch + 2 * x] = expandedPixel;
      dest[y * 2 * destPitch + 2 * x + 1] = expandedPixel;
      dest[(y * 2 + 1) * destPitch + 2 * x] = expandedPixel;
      dest[(y * 2 + 1) * destPitch + 2 * x + 1] = expandedPixel;
    }
  }
}

#define SRC_WIDTH 160
#define SRC_HEIGHT 80
#define DEST_WIDTH 320
#define DEST_HEIGHT 160

void sprintfBinary(uint8_t num, char *buf)
{
  sprintf(buf, "0b");
  buf += 2;
  for (int i = 7; i >= 0; i--)
  {
    uint8_t bit = (num >> i) & 1;
    sprintf(buf, "%d", bit);
    buf++;
  }
}

// eggfly
void enlargeBuffer(uint8_t *src, uint8_t *dest)
{
  memset(dest, 0, DEST_WIDTH * DEST_HEIGHT / 8);
  for (int src_y = 0; src_y < SRC_HEIGHT; src_y++)
  {
    for (int x = 0; x < SRC_WIDTH; x += 8)
    {
      int src_offset = src_y * SRC_WIDTH / 8 + x / 8;
      uint8_t srcByte = src[src_offset];
      for (int i = 0; i < 8; i++)
      {
        uint8_t pixel = (srcByte >> (7 - i)) & 1;
        int src_x = x + i;

        int dest_x0 = 2 * src_x;
        int dest_y0 = 2 * src_y;
        int dest_x1 = dest_x0 + 1;
        int dest_y1 = dest_y0;
        int dest_x2 = dest_x0;
        int dest_y2 = dest_y0 + 1;
        int dest_x3 = dest_x0 + 1;
        int dest_y3 = dest_y0 + 1;

        int dest_offset_0 = dest_y0 * DEST_WIDTH / 8 + dest_x0 / 8;
        int dest_offset_1 = dest_y1 * DEST_WIDTH / 8 + dest_x1 / 8;
        int dest_offset_2 = dest_y2 * DEST_WIDTH / 8 + dest_x2 / 8;
        int dest_offset_3 = dest_y3 * DEST_WIDTH / 8 + dest_x3 / 8;

        dest[dest_offset_0] |= (pixel << (7 - dest_x0 % 8));
        dest[dest_offset_1] |= (pixel << (7 - dest_x1 % 8));
        dest[dest_offset_2] |= (pixel << (7 - dest_x2 % 8));
        dest[dest_offset_3] |= (pixel << (7 - dest_x3 % 8));
        //        if (times % 10 == 0) {
        //          if (pixel) {
        //            char buf1[12];
        //            sprintfBinary(dest[dest_offset_0], buf1);
        //            char buf2[12];
        //            sprintfBinary(dest[dest_offset_1], buf2);
        //            char srcByteBuf[12];
        //            sprintfBinary(srcByte, srcByteBuf);
        //            Serial.printf("%d,%d=%d,src_byte[%d]=%s,dest->(%d,%d)(%d,%d),dest[%d]=%s,dest[%d]=%s\n",
        //                          src_x, src_y, pixel, src_offset, srcByteBuf,
        //                          dest_x0, dest_y0, dest_x1, dest_y1,
        //                          dest_offset_0, buf1, dest_offset_1, buf2);
        //          }
        //        }
      }
    }
  }
}

void magnify_pixels(uint8_t *src, uint8_t *dest)
{
  for (int y = 0; y < 80; y++)
  {
    for (int x = 0; x < 160; x++)
    {
      uint8_t pixel = ((src[y * 20 + (x / 8)] >> (7 - (x % 8))) & 0x01) ? 0xFF : 0x00;
      for (int i = 0; i < 2; i++)
      {
        for (int j = 0; j < 2; j++)
        {
          dest[(y * 320 + x * 2 + i) * 4 + j] = pixel;
        }
      }
    }
  }
}

void my_lcd_zoom_in(uint8_t *src, uint8_t *dest)
{
  // auto t = millis();
  for (size_t x = 0; x < 160; x++)
  {
    for (size_t y = 0; y < 80; y++)
    {
      size_t src_offset = y * 160 * 2 + 2 * x;
      uint8_t v1 = src[src_offset];
      uint8_t v2 = src[src_offset + 1];
      size_t dest_offset_0 = 640 * (2 * y) + 2 * (2 * x);
      size_t dest_offset_1 = 640 * (2 * y + 1) + 2 * (2 * x);
      size_t dest_offset_2 = 640 * (2 * y) + 2 * (2 * x + 1);
      size_t dest_offset_3 = 640 * (2 * y + 1) + 2 * (2 * x + 1);
      dest[dest_offset_0] = v1;
      dest[dest_offset_0 + 1] = v2;
      dest[dest_offset_1] = v1;
      dest[dest_offset_1 + 1] = v2;
      dest[dest_offset_2] = v1;
      dest[dest_offset_2 + 1] = v2;
      dest[dest_offset_3] = v1;
      dest[dest_offset_3 + 1] = v2;
    }
  }
  // cost 7 ms
  // Serial.printf("zoom cost: %dms\n", millis() - t);
}

// Twice for more functions
uint8_t func_keys[] = {
    8,
    8,
    9,
    9,
    10,
    10,
    11,
    11,
    12,
    12,
    13,
    13,
    14,
    14,
};
const size_t func_key_size = sizeof(func_keys) / sizeof(func_keys[0]);
int curr_key_index = 0;
int key_release_countdown = 0;
int key_to_release = -1;

inline void nc1020_loop()
{
  auto start_time = millis();
  size_t slice = 20; // origin is 20, can be 10, 15
  wqx::RunTimeSlice(slice, false);
  if (LOG_LEVEL <= LOG_LEVEL_VERBOSE)
  {
    Serial.printf("slice=%d,cost=%dms\n", slice, millis() - start_time);
  }
  // }
  wqx::CopyLcdBuffer((uint8_t *)lcd_buff);
  // memset(lcd_buff_ex, 0xf0, sizeof(lcd_buff_ex));
  enlargeBuffer(lcd_buff, lcd_buff_expanded);
  // M5.Lcd.drawBitmap(80, 80, 160, 80, (uint8_t*)lcd_buff_ex);
  // canvas.fillRect(0, (172 - 160) / 2, 320, 160, WQX_COLOR_RGB565_BG);
  // canvas.fillScreen(RGB888_TO_RGB565(0x20, 0x20, 0x20));
  canvas.fillScreen(RGB565_RED);
  // canvas.fillScreen(RGB565_WHITE);
  canvas.drawBitmap(
      0,
      (172 - 160) / 2,
      lcd_buff_expanded, 320, 160, WQX_COLOR_RGB565_FG, WQX_COLOR_RGB565_BG);

  // canvas.drawBitmap(0, 0, lcd_buff, 160, 80, WQX_COLOR_RGB565_FG, WQX_COLOR_RGB565_BG);

  canvas.flush();

  if (key_to_release > 0)
  {
    if (key_release_countdown == 0)
    {
      wqx::SetKey(key_to_release, false);
      Serial.printf("nc1020 set release key: %d\n", key_to_release);
      key_to_release = -1;
    }
    else
    {
      key_release_countdown--;
    }
  }
  else
  {
    // char key = 0;
    if (ruler_deck::pressed_key.length() > 0)
    {
      int keycode = 0;
      if (nc1020_keymap.find(ruler_deck::pressed_key) != nc1020_keymap.end())
      {
        keycode = nc1020_keymap[ruler_deck::pressed_key];
        Serial.printf("receive key=%s -> 0x%02x\n", ruler_deck::pressed_key.c_str(), keycode);
      }
      else
      {
        Serial.printf("receive key=%s -> not found!\n", ruler_deck::pressed_key.c_str());
        keycode = -1;
      }
      wqx::SetKey(keycode, true);
      key_to_release = keycode;
      key_release_countdown = 1;
      // Serial.printf("BtnA, idx=%d\n", curr_key_index);
      curr_key_index++;
      curr_key_index %= func_key_size;
    }
  }
  if (LOG_LEVEL <= LOG_LEVEL_VERBOSE)
  {
    Serial.printf("nc1020_loop,slice=%d,cost=%dms\n", slice, millis() - start_time);
  }
}

void loop()
{
  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  Serial.println("I'm the master processor!");
  Serial.printf("ESP32 Chip model = % s Rev % d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has % d cores\n", ESP.getChipCores());
  Serial.print("Chip ID: ");
  Serial.println(chipId);

  // delay(3000);
  for (;;)
  {
    currPage->onDraw();
    keyboard_loop();
    // grub_loader_menu();
    // nc1020_loop();
    // gfx_demo();
    // delay(100);
  }
}

void handle_backlight_key()
{
  uint8_t count = sizeof(backlight_levels) / sizeof(backlight_levels[0]);
  current_backlight_level++;
  current_backlight_level %= count;
  set_backlight_level();
}

void show_restart()
{
  portENTER_CRITICAL(&my_mutex);
  canvas.fillScreen(0);
  canvas.setCursor(0, 0);
  canvas.setTextColor(0xFF);
  canvas.setTextSize(2);
  canvas.println("---- - RESTART NOW ! ---- -");
  portEXIT_CRITICAL(&my_mutex);
  // delay(2000);
}

void select_boot_item(int8_t offset)
{
  int8_t new_selected = selected_menu_index + offset;
  if (new_selected < 0)
  {
    new_selected = 0;
  }
  else if (new_selected >= menu_items_count)
  {
    new_selected = menu_items_count - 1;
  }
  selected_menu_index = new_selected;
}

bool keyboard_callback(const char *key_str)
{
  Serial.printf("keyboard_callback: % s\n", key_str);
  if (strcmp(key_str, "Backlight") == 0)
  {
    handle_backlight_key();
    return true;
  }
  else if (strcmp(key_str, "Power") == 0)
  {
    show_restart();
    ESP.restart();
    return true;
  }
  else
  {
    return currPage->handleKey(key_str);
  }
}
