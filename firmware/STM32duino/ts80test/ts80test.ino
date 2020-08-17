/*  
 Basic Arduino music player for TS80 (it works!) also works for Bluepill
 Install stm32duino as here https://github.com/stm32duino/Arduino_Core_STM32
 Select Board: "Generic STM32F1 Series"
 Board part number: "Generic F103T8"
 Upload method: "Maple DFU Bootloader 2.0"
 Edit %LOCALAPPDATA%\Arduino15\packages\STM32\hardware\stm32\1.9.0\platform.txt:
 build.preferred_out_format=hex
 Edit %LOCALAPPDATA%\Arduino15\packages\STM32\hardware\stm32\1.9.0\boards.txt:
 GenF1.menu.upload_method.dfu2Method.build.flash_offset=0x4000
 Use Sketch->Export Compiled binary to save hex in the sketch folder
 Upload via DFU bootloader, make sure it renames to RDY, power cycle the iron
*/

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
  display.print("Arduino!");
  display.display();  
#endif
  
}

void loop() {
  static uint8_t flag;
  digitalWrite(LEDPIN, flag ? HIGH : LOW);
  flag ^= 1;
  delay(50);
}
