/*

  Sigma Delta DAC example using DMA.

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

  v0.1 4.4.2017  C. -H-A-B-E-R-E-R-  initial version

  It is mandatory to keep the list of authors in this code.
  Please add your name if you improve/extend something

  modified by joric for the ts80player project
  https://github.com/joric/ts80player/

*/

#include "stm32f103.h"

#define DMABUFFERLENGTH 100

volatile uint8_t DmaBuffer[DMABUFFERLENGTH];

// good DMA to port article:
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


#define MAXVALUE 255

#define PIN_PA0 6 // speaker pin
#define PORTA_PINMASK (1<<PIN_PA0) 

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

uint32_t DacSamplingFrequency;

uint16_t SineTable[256];

void setup()
{
  pinMode(PC13, OUTPUT);
  pinMode(PA6, OUTPUT); // speaker pin
  Serial.begin(115200);
  Serial.println("hello");


  startDMA();

  uint32_t n;

  uint32_t startTime=micros();
  for (n = 0; n < 1000; n++) writeDac(0);
  uint32_t stopTime=micros();
  
  Serial.print("DAC write time for 1000 samples in us: ");Serial.println(stopTime-startTime);
  DacSamplingFrequency=1000000UL*1000/(stopTime-startTime);
  Serial.print("DAC sampling frequency [Hz]: ");Serial.println(DacSamplingFrequency);

  for(n=0;n<256;n++) SineTable[n]= (sin(2*PI*n/255)+1)*100;

}

#define LEDPIN 13

uint8_t Counter=0;


void loop()
{
  /*
  GPIOC->BSRR = 1 << LEDPIN; // on
  delay(100);
  
  GPIOC->BSRR = 1 << LEDPIN + 16; // off
  delay(100);
*/

  
  writeDac(SineTable[Counter]);

  //Counter++;
  uint32_t f_Hz=440;
  Counter+=256*f_Hz/DacSamplingFrequency;

}
