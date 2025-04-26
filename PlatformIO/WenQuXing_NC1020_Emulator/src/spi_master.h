#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#include "freertos/queue.h"
#include "freertos/portmacro.h"
#include "string"

#define SCREEN_WIDTH    480
#define SCREEN_HEIGHT   320

void set_buffer(uint8_t* tx, uint8_t* rx, const size_t size) {
  if (tx) {
    for (uint32_t i = 0; i < size; i++) {
      tx[i] = 0xFF;
      // tx[i] = i & 0xFF;
    }
    // tx[19200 - 1] = 0x00;
  }
  if (rx) {
    memset(rx, 0, size);
  }
}


static const uint32_t BUFFER_SIZE = 19200;
static const uint32_t KEYPAD_BUFFER_SIZE = 8 * 8;

uint8_t* spi_master_tx_buf;
uint8_t* spi_master_rx_buf;


const byte ROWS = 8; // rows
const byte COLS = 8; // columns

uint8_t seq = 0;

// 2 swap buffers
uint8_t keyStates[2][ROWS][COLS];
uint8_t currentKeyStateIndex = 0;

// uint8_t keyStates[ROWS][COLS];

std::string screen_str;

const char PROGMEM keys[8][8][10] = {
  {"Power", "F1", "F2", "F3", "F4", "F5", "F6", "Backlight"},
  {"1", "2", "3", "4", "5", "6", "7", "8"},
  {"Q", "W", "E", "R", "T", "Y", "U", "I"},
  {"A", "S", "D", "F", "G", "H", "J", "K"},
  {"Shift", "Z", "X", "C", "V", "B", "N", "M"},
  {"Catalogue", "Express", "Follow", "Font", "USB", "Exit", "Speak",  "Save"},
  {"Space", "Enter", "Up", "Down", "Left", "Right", "PageUp", "PageDown"},
  {"9", "0", "-", "O", "P", "L", "Input", "Delete"},
};

bool keyboard_callback(const char* key) __attribute__((weak));

void printPressedKeys(uint8_t prev, uint8_t curr) {
  for (size_t i = 0; i < ROWS; i++) {
    for (size_t j = 0; j < COLS; j++) {
      auto prevState = keyStates[prev][i][j];
      auto currState = keyStates[curr][i][j];
      if ((prevState == 'p' || prevState == 'r') && (currState == 'p' || currState == 'r')) {
        if (prevState != currState) {
          Serial.printf("\"%s\"(%d,%d)=%c, cb=%p\n", keys[i][j], i, j, currState, keyboard_callback);
          if (currState == 'p') {
            bool handled = false;
            if (keyboard_callback) {
              handled = keyboard_callback(keys[i][j] + 0);
            }
            if (!handled) {
              // screen_str += keys[i][j];
            }
            // screen_str += ", ";
          }
        }
      }
    }
  }
}

void spi_keyboard_handle_loop() {
  uint8_t nextKeyStateIndex = currentKeyStateIndex == 0 ? 1 : 0;
  memcpy(keyStates[nextKeyStateIndex], spi_master_rx_buf, ROWS * COLS);
  printPressedKeys(currentKeyStateIndex, nextKeyStateIndex);
  currentKeyStateIndex = nextKeyStateIndex;
}

#endif  // SPI_MASTER_H
