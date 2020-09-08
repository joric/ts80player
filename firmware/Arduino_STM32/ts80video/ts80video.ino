#include <USBComposite.h>

USBCompositeSerial CompositeSerial;

#define DISPLAY_ENABLED 1

#if DISPLAY_ENABLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_I2C_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
#endif


void setup() {
  USBComposite.setProductId(0x29);
  CompositeSerial.registerComponent();
  USBComposite.begin();

#if DISPLAY_ENABLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(0);
  display.setTextColor(WHITE);  
  display.setCursor(0,16);
  display.print("Waiting data...");
  display.display();  
#endif  
}

#define W 96
#define H 16
#define SIZE (W*H/8)
static int ofs = 0;
uint8_t buf[SIZE];
static int frame = 0;

void loop() {

    while (CompositeSerial.available()) {
        buf[ofs] = CompositeSerial.read();
        ofs = (ofs + 1) % SIZE;
        if (ofs == 0) {       
          display.clearDisplay();
          display.drawBitmap(0,16, buf, W, H, 1);
          //display.setCursor(0,16);
          //display.print(frame);
          display.display();         
          frame++;
        } 
    }
}
