#define LEDPIN     PA6 // audio output (TS80 heater pin)

#define DISPLAY_ENABLED 1 // disable in case of ROM/RAM overflow

#if DISPLAY_ENABLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_I2C_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire);
#endif

extern "C" void SystemClock_Config(void) {
  // there's no external clock on TS80, we need this handler empty
}

void setup() {
  pinMode(LEDPIN, OUTPUT);

#if DISPLAY_ENABLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(0);
  display.setTextColor(WHITE);   
  display.setCursor(0,0);
  display.setCursor(0,16);
  display.print("Playing...");
  display.display();  
#endif
  
}

void beep(int freq, int ms) {  
  freq *= 2; // freq fix for two poles  
  static uint8_t flag;
  unsigned long delta = 1000000 / freq;
  unsigned long start = micros();
  unsigned long end = start + ms * 1000;
  while (micros() < end) {
    unsigned long next = micros() + delta;
    digitalWrite(LEDPIN, flag ? HIGH : LOW);
    flag ^= 1;
    while (micros() < next) {
      delayMicroseconds(1);      
    }
  }  
}


#include "bumble.h"

void loop() {
  const byte * p = bumble_data;
  byte rate = *p++;
  for (int i=0; *p; i++) {
      byte w = *p++;
      byte l = *p++;
      byte h = *p++;
      int freq = 65536 * 64 / (h * 256 + l);
      int ms = w * 10;
      beep(freq, ms);
      delay(ms*0.1);
  }
}
