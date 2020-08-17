/*

  Sigma Delta DAC example using DMA and double buffering

  Generate a sine wave, fill a buffer and play the samples in a timer driven interrupt routine.

  Hardware: Poti on analog input 0
  Library dependency: you need the libmaple hardware timer
  http://docs.leaflabs.com/docs.leaflabs.com/index.html  

  This is just an experiment using a buffer filled with the sigmal delta principle
  and output it by the DMA to port PB0.

  The DMA is programmed bare metal without any wrapper functions.

  https://github.com/ChrisMicro/BluePillSound

  ************************************************************************

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  ********************* list of outhors **********************************

  v0.1  4.4.2017  C. -H-A-B-E-R-E-R-  initial version
  v0.2  5.4.2017  C. -H-A-B-E-R-E-R-  higher sampling rate due to 32bit table
  v0.3  6.4.2017  C. -H-A-B-E-R-E-R-  interrupt driven sine wave oscillator
  v0.3 18.4.2017  C. -H-A-B-E-R-E-R-  buffered player

  It is mandatory to keep the list of authors in this code.
  Please add your name if you improve/extend something

  modified by joric for the ts80player project
  https://github.com/joric/ts80player/
*/

#include "stm32f103.h"

#define DISPLAY_ENABLED 1 // disable in case of ROM/RAM overflow

#if DISPLAY_ENABLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_I2C_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire);
#endif

#define DACSAMPLINGRATE_HZ    32000 // worked as 32000 with buffer 64
#define TIMER_INTTERUPT_US    1000000UL / DACSAMPLINGRATE_HZ    

HardwareTimer timer(2);

#define DMABUFFERLENGTH 64 // 100 is too slow, 32 is too clicky

volatile uint32_t DmaBuffer[DMABUFFERLENGTH];


#define PWM_FREQ (DACSAMPLINGRATE_HZ)
//#define FATAL_PT3 1
//#define APPLE_PT3 1 // 6-channel, super slow
//#include "pt3_play_shiru_cpp.h" // legacy C++ player for testing
#include "pt3_play_shiru.h" // plain C player
//#include "pt3_play_zxssk.h" // way too slow (max ~16khz)


// good DMA article:
// https://vjordan.info/log/fpga/stm32-bare-metal-start-up-and-real-bit-banging-speed.html
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

// maple timer
// http://docs.leaflabs.com/docs.leaflabs.com/index.html
void startTimer()
{
  // Pause the timer while we're configuring it
  timer.pause();

  // Set up period
  timer.setPeriod(TIMER_INTTERUPT_US); // in microseconds

  // Set up an interrupt on channel 1
  timer.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  timer.setCompare(TIMER_CH1, 1);  // Interrupt 1 count after each update
  timer.attachCompare1Interrupt(DACsampleRateHandler);

  // Refresh the timer's count, prescale, and overflow
  timer.refresh();

  // Start the timer counting
  timer.resume();
}

#define SPEAKERPIN PA6            // speaker pin, be sure that it corresponds to the DMA pin setup in the next line
#define PIN_PA 6                  // speaker pin, you can choose 0..7, 0 means PB0, 1 means PB1 ...
#define PORTA_PINMASK (1<<PIN_PA)

#define FAST_DELTA_SIGMA 1

#if FAST_DELTA_SIGMA
uint32_t CoarseSigmaDeltaTable[] = {0x00000000 * PORTA_PINMASK, 0x01000000 * PORTA_PINMASK, 0x01000100 * PORTA_PINMASK, 0x01010100 * PORTA_PINMASK, 0x01010101 * PORTA_PINMASK};

// Sigma Delta DAC
// input value 10 bit 0..1023
void writeDac( uint32_t sollwert )
{
  uint32_t  integrator = 0;
  static uint32_t  oldValue = 0;

  uint32_t n;

  // sigma delta DAC
  for (n = 0; n < DMABUFFERLENGTH / 4; n++)
  {
    integrator += sollwert - oldValue;
    oldValue = integrator & 0b11100000000;
    DmaBuffer[n] = CoarseSigmaDeltaTable[ oldValue >> 8 ];
  }
}

#else // slow delta-sigma

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
#endif

typedef uint16_t buf_t;
#define SOUNDBUFFERLENGTH (DMABUFFERLENGTH) //really not sure how much we need here. safety gap? //joric

volatile buf_t SoundBuffer0[SOUNDBUFFERLENGTH];
volatile buf_t SoundBuffer1[SOUNDBUFFERLENGTH];

volatile buf_t * volatile SoundBufferCurrentlyPlaying;

void DACsampleRateHandler()
{
  static uint32_t bufferIndex=0;
  
  writeDac(SoundBufferCurrentlyPlaying[bufferIndex++]);
  
  if( bufferIndex >= SOUNDBUFFERLENGTH )
  {
    bufferIndex=0;
    if( SoundBufferCurrentlyPlaying != SoundBuffer1 )SoundBufferCurrentlyPlaying = SoundBuffer1;
    else                                    SoundBufferCurrentlyPlaying = SoundBuffer0;
  }
 
}
#define LEDPIN     PC13 // Blue Pill LED
#define INITLED    pinMode(LEDPIN, OUTPUT)

#define LEDON      digitalWrite(LEDPIN, HIGH)
#define LEDOFF     digitalWrite(LEDPIN, LOW)

void toggleLed()
{
  static uint8_t flag=0;

  if(flag) LEDON;
  else     LEDOFF;

  flag^=1;
}

volatile buf_t * volatile SoundBuffer;

void setup()
{
  uint32_t n;

  pt3_init();
  
  pinMode(PC13, OUTPUT);
  pinMode(SPEAKERPIN, OUTPUT); 
  Serial.begin(115200);
  Serial.println("SPI SIGMA DELTA SOUND calculation performance");

  delay(100);
  SoundBufferCurrentlyPlaying = SoundBuffer0;
  SoundBuffer                 = SoundBuffer1;
  startDMA();

  // ** just check the write speed **
  uint32_t startTime = micros();
  for (n = 0; n < 1000; n++) writeDac(0); // dummy write silence
  uint32_t stopTime = micros();

  uint32_t DacSamplingFrequency;
  Serial.print("DAC write time for 1000 samples in us: "); Serial.println(stopTime - startTime);
  DacSamplingFrequency = 1000000UL * 1000 / (stopTime - startTime);
  Serial.print("maximum possible DAC sampling frequency [Hz]: "); Serial.println(DacSamplingFrequency);

  // *********************************

  startTimer();

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

volatile uint32_t Oscillator1_Frequency_Hz=440;

void updateSound()
{
  for(uint32_t n=0;n<SOUNDBUFFERLENGTH;n++)
  { 
    SoundBuffer[n]=pt3_play();
  }

}

void loop()
{
  SoundBuffer = SoundBufferCurrentlyPlaying; 
  while(SoundBuffer == SoundBufferCurrentlyPlaying); // wait until buffer is emtpy
  
  //LEDON; // debug led for processing time measuremt with oscilloscope
  updateSound();
  //LEDOFF;

}
