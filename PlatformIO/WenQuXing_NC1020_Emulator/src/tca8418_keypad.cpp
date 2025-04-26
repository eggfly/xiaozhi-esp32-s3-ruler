#include <Adafruit_TCA8418.h>
#include <string>
#include "app.h"
#include <map>

Adafruit_TCA8418 keypad;

#define ROWS 7
#define COLS 5

std::string ruler_deck::pressed_key;

//  typical Arduino UNO
const int IRQPIN = 8;

volatile bool TCA8418_event = false;

void TCA8418_irq()
{
  TCA8418_event = true;
}

const char *keymap[ROWS][COLS] = {
    {"Q", "W", "E", "R", "T"},
    {"Y", "U", "I", "O", "P"},
    {"A", "S", "D", "F", "G"},
    {"H", "J", "K", "L", "Backspace"},
    {"Shift", "Z", "X", "C", "V"},
    {"B", "N", "M", "'", "Enter"},
    {"F1", "F2", "Space", "F3", "F4"},
};

std::map<std::string, std::string> func_decorator_map = {
    {"Q", "P1"},
    {"W", "P2"},
    {"E", "P3"},
    {"R", "P4"},
    {"T", "P5"},
    {"Y", "P6"},
    {"U", "P7"},
};

void keyboard_setup()
{
  Serial.println(__FILE__);
  // Set IO 0 as function select key
  pinMode(0, INPUT);
  Wire.begin(1, 2);
  if (!keypad.begin(TCA8418_DEFAULT_ADDR, &Wire))
  {
    Serial.println("keypad not found, check wiring & pullups!");
    while (1)
      ;
  }

  //  configure the size of the keypad matrix.
  //  all other pins will be inputs
  keypad.matrix(ROWS, COLS);

  //  install interrupt handler
  //  going LOW is interrupt
  pinMode(IRQPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IRQPIN), TCA8418_irq, CHANGE);

  //  flush pending interrupts
  keypad.flush();
  //  enable interrupt mode
  keypad.enableInterrupts();
}

void keyboard_loop()
{
  if (TCA8418_event)
  {
    //  datasheet page 15 - Table 1
    int k = keypad.getEvent();

    //  try to clear the IRQ flag
    //  if there are pending events it is not cleared
    keypad.writeRegister(TCA8418_REG_INT_STAT, 1);
    int intstat = keypad.readRegister(TCA8418_REG_INT_STAT);
    if ((intstat & 0x01) == 0)
      TCA8418_event = false;
    bool is_press = (k & 0x80);
    if (is_press)
    {
      Serial.print("PRESS\tROW: ");
    }
    else
    {
      Serial.print("RELEASE\tROW: ");
    }
    k &= 0x7F;
    k--;
    uint8_t row = k / 10;
    uint8_t col = k % 10;
    Serial.print(row);
    Serial.print("\tCOL: ");
    Serial.print(col);
    Serial.print(" - ");
    Serial.print(keymap[row][col]);
    Serial.println();
    bool func_decorator = digitalRead(0) == LOW;
    Serial.printf("IO0: func_decorator: %d\n", func_decorator);
    if (is_press)
    {
      if (func_decorator && func_decorator_map.find(keymap[row][col]) != func_decorator_map.end())
      {
        ruler_deck::pressed_key = func_decorator_map[keymap[row][col]];
      }
      else
      {
        ruler_deck::pressed_key = keymap[row][col];
      }
    }
    else
    {
      ruler_deck::pressed_key = "";
    }
  }

  // other code here
  // delay(10);
}
