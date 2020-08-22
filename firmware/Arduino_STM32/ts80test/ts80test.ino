// #define LEDPIN     PC13 // Blue Pill LED
#define LEDPIN     PA6 // PWM_OUT (TS80 heater pin)


#define DISPLAY_ENABLED 1 // disable in case of ROM/RAM overflow

#if DISPLAY_ENABLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_I2C_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire);
#endif


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

void loop() {
  static uint8_t flag;
  digitalWrite(LEDPIN, flag ? HIGH : LOW);
  flag ^= 1;
  delay(50);
}
