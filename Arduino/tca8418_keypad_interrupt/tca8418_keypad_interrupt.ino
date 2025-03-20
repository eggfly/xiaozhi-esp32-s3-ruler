
/***************************************************

  @file tca8418_keypad_interrupt.ino

  This is an example for the Adafruit TCA8418 Keypad Matrix / GPIO Expander Breakout

  Designed specifically to work with the Adafruit TCA8418 Keypad Matrix
  ----> https://www.adafruit.com/products/XXXX

  These Keypad Matrix use I2C to communicate, 2 pins are required to
  interface.
  The Keypad Matrix has an interrupt pin to provide fast detection
  of changes. This example shows the working of polling.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/


#include <Adafruit_TCA8418.h>

Adafruit_TCA8418 keypad;

#define ROWS 7
#define COLS 5

//  typical Arduino UNO
const int IRQPIN = 8;

volatile bool TCA8418_event = false;

void TCA8418_irq()
{
  TCA8418_event = true;
}


const char* keymap[ROWS][COLS] = {
  {"Q", "W", "E", "R", "T"},
  {"Y", "U", "I", "O", "P"},
  {"A", "S", "D", "F", "G"},
  {"H", "J", "K", "L", "Backspace"},
  {"Shift", "Z", "X", "C", "V"},
  {"B", "N", "M", "'", "Enter"},
  {"F1", "F2", "Space", "F3", "F4"},
};


void setup()
{
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println(__FILE__);
  Wire.begin(1, 2);
  if (! keypad.begin(TCA8418_DEFAULT_ADDR, &Wire)) {
    Serial.println("keypad not found, check wiring & pullups!");
    while (1);
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


void loop()
{
  if (TCA8418_event == true)
  {
    //  datasheet page 15 - Table 1
    int k = keypad.getEvent();

    //  try to clear the IRQ flag
    //  if there are pending events it is not cleared
    keypad.writeRegister(TCA8418_REG_INT_STAT, 1);
    int intstat = keypad.readRegister(TCA8418_REG_INT_STAT);
    if ((intstat & 0x01) == 0) TCA8418_event = false;

    if (k & 0x80) Serial.print("PRESS\tR: ");
    else Serial.print("RELEASE\tROW: ");
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
  }

  // other code here
  delay(10);
}
