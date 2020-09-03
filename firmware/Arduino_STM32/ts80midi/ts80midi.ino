#define LEDPIN     PA6 // audio output (TS80 heater pin)
#define BUTTON_A   PB1
#define BUTTON_B   PB0

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
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_A), btnA, FALLING);  
  attachInterrupt(digitalPinToInterrupt(BUTTON_B), btnB, FALLING);  
 
  
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

const byte * data = 0;
int pos, notes, rate, tune;
String name;

void update(int pos, int notes) {
#if DISPLAY_ENABLED
  display.clearDisplay();
  display.setCursor(0,16);
  display.print(name);  
  display.setCursor(0,24);
  display.print("< Pos: ");
  if (pos<10) display.print(" ");
  if (pos<100) display.print(" ");
  display.print(pos);
  display.print("/");
  display.print(notes);
  display.print(" >");
  display.display();
#endif
}

#include "brigada.h"
#include "boomer.h"
#include "sept.h"
#include "chopin.h"
#include "bach.h"
#include "bumble.h"

enum {
  brigada,
  boomer,
  sept,
  chopin,
  bach,
  bumble,
  //
  TUNES
};

#define T(id) if (tune == id) { data = id ## _data;  notes = (sizeof(id ## _data)-2)/3; name = #id; } else

void load_data(int tune) {
  T(brigada);
  T(boomer);
  T(sept);
  T(chopin);
  T(bach);
  T(bumble);
}

void select(int x) {
  pos = 0;  
  tune = (x + TUNES) % TUNES;
  load_data(tune);
}

int dir = 0;

void btnA() {
  if (dir==0) dir = 1;
}

void btnB() {
  if (dir==0) dir = -1;
}

void loop() {
  if (!data) {
    select(0);
  }

  if (dir!=0) {
    select(tune+dir);    
    delay(200);
    dir = 0;    
  }
    
  update(pos, notes);  

  byte w = data[pos*3+1];
  byte lo = data[pos*3+2];
  byte hi = data[pos*3+3];

  int freq = 65536 * 32 / (hi * 256 + lo);
  int ms = w * 15;
  beep(freq, ms);
  
  if (pos!=-1) pos = (pos+1) % notes;
}
