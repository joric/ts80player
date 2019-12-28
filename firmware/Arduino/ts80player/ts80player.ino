#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_I2C_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire);

#include "stm32f103.h"

#include "pt3_includes.h"

#define DMABUFFERLENGTH 32 // speed heavily depends of that. don't know how to fix

volatile uint8_t DmaBuffer[DMABUFFERLENGTH];

void startDMA()
{
  RCC->AHBENR |= RCC_AHBENR_DMA1EN; // enable DMA

  // channel 1: mem:8bit -> peri:8bit
  DMA1_Channel1->CNDTR = DMABUFFERLENGTH;
  DMA1_Channel1->CMAR = (uint32_t)DmaBuffer;
  DMA1_Channel1->CPAR = (uint32_t) & (GPIOA->ODR);
  DMA1_Channel1->CCR = 0;
  DMA1_Channel1->CCR = DMA_CCR1_MEM2MEM | DMA_CCR1_PL | DMA_CCR1_MINC | DMA_CCR1_CIRC | DMA_CCR1_DIR | DMA_CCR1_EN;
}

#define PIN_PA0 6 // speaker pin, you can choose 0..7, don't forgett to adjust the pin initializaion in setup when you change this value
#define PORTA_PINMASK (1<<PIN_PA0) 

#define MAXVALUE 255
// Sigma Delta DAC
void writeDac( uint32_t sollwert )
{
  int32_t  integrator = 0;
  static uint32_t  oldValue = 0;

  uint32_t n;
  
  // sigma delta DAC, hold the DAC value for n-steps constant
  for (n = 0; n < DMABUFFERLENGTH; n++)
  {
    integrator += sollwert - oldValue;
    if (integrator > 0)
    {
      oldValue = MAXVALUE;
      DmaBuffer[n]=PORTA_PINMASK;
    }
    else
    {
      oldValue = 0;
      DmaBuffer[n]=0;
    }
  }
}

void setup()
{  
  pinMode(PC13, OUTPUT);
  pinMode(PA6, OUTPUT); // speaker pin

  pt3_init();

  startDMA();

  uint32_t n;

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(0);
  display.setTextColor(WHITE);   
  display.setCursor(0,0);
  display.setCursor(0,16);
  display.print("Playing...");
  display.display();

}

void loop()
{
  writeDac(pt3_play());
}
