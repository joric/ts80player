#include <USBMassStorage.h>

#define DISPLAY_ENABLED 1

#if (DISPLAY_ENABLED)
// SSD1306Ascii is smaller than Adafruit
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
SSD1306AsciiWire oled;
#endif

USBMassStorage MassStorage;

#include "image.h" //8kb drive

bool write(const uint8_t *writebuff, uint32_t memoryOffset, uint16_t transferLength) {
  memcpy(image+SCSI_BLOCK_SIZE*memoryOffset, writebuff, SCSI_BLOCK_SIZE*transferLength);
  return true;
}

bool read(uint8_t *readbuff, uint32_t memoryOffset, uint16_t transferLength) {
  memcpy(readbuff, image+SCSI_BLOCK_SIZE*memoryOffset, SCSI_BLOCK_SIZE*transferLength);  
  return true;
}

void setup() {  
  MassStorage.setDriveData(0, sizeof(image)/SCSI_BLOCK_SIZE, read, write);
  MassStorage.registerComponent();  
  MassStorage.begin();

#if (DISPLAY_ENABLED)
  Wire.begin();
  Wire.setClock(400000L);
  oled.begin(&Adafruit128x32, 0x3C);
  oled.setFont(Adafruit5x7);
  oled.println();
  oled.println();
  oled.print("USB disk");  
#endif  
}

void loop() {
  MassStorage.loop();   
}
