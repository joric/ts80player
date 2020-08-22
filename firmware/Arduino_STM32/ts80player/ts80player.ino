#include "stm32f103.h"

#define DISPLAY_ENABLED 1 // disable in case of ROM/RAM overflow

#if DISPLAY_ENABLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_I2C_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
#endif

#define DACSAMPLINGRATE_HZ    16000 // worked as 32000 with buffer 64 (nucleo needs 16000)
#define TIMER_INTTERUPT_US    1000000UL / DACSAMPLINGRATE_HZ    

HardwareTimer timer(2);

#define DMABUFFERLENGTH 64 // 100 is too slow, 32 is too clicky

volatile uint32_t DmaBuffer[DMABUFFERLENGTH];

#define PWM_FREQ (DACSAMPLINGRATE_HZ)
//#define FATAL_PT3 1
//#define APPLE_PT3 1 // 6-channel, super slow
//#include "pt3_play_shiru_cpp.h" // legacy C++ player for testing
#include "pt3_play_shiru.h" // plain C player
//#include "pt3_play_zxssk.h" // best quality but way too slow (max ~16khz)


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

#define PIN_OLED_RST 15
#define OLED_RST_PINMASK (1<<PIN_OLED_RST)

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

    // trying to prevent TS80 OLED reset via PA15
    // sound pin: PA6, mask: 1<<6 = 0x40 
    // OLED reset PA15, mask: 1<<15 = 0x8000

    // 0x00008000 does reset OLED, sound present
    // 0x00000040 does reset OLED, no sound    
    // 0xffffffff does not reset OLED
    // 0x0000ffff does reset OLED
    // 0xffff0000 does reset OLED
    // 0x000000ff does reset OLED
    // 0xfffffff0 does not (no sound)
    // 0xffffff00 does not (no sound)
    // 0xfffff000 does not (no sound)
    // 0xffff0000 does reset (no sound)
    // 0x00ffff00 does reset (no sound)
    // 0x0ffffff0 does not (no sound)
    // 0xff0000ff does reset (no sound)
    // 0xffff0fff does not (no sound)
    // 0xffffe000 does not (no sound)
    // 0xffff8000 does not (no sound)
    // 0xff000000 does reset (no sound)

    // I don't know what the hell is going on so far

    DmaBuffer[n] = CoarseSigmaDeltaTable[ oldValue >> 8 ];

    //DmaBuffer[n] = 0xffff8000  | CoarseSigmaDeltaTable[ oldValue >> 8 ];
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

  // NO SOUND HERE AT ALL
  
  // sigma delta DAC, hold the DAC value for n-steps constant
  for (n = 0; n < DMABUFFERLENGTH; n++)
  {
    integrator += sollwert - oldValue;
    if (integrator > 0)
    {
      oldValue = MAXVALUE;
      DmaBuffer[n] = 0xff ^ PORTA_PINMASK;
    }
    else
    {
      oldValue = 0;
      DmaBuffer[n] = 0xff;
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

volatile buf_t * volatile SoundBuffer;

void setup()
{
  uint32_t n;
  pt3_init();

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
 
  pinMode(SPEAKERPIN, OUTPUT); 
  //Serial.begin(115200);
  //Serial.println("SPI SIGMA DELTA SOUND calculation performance");

  delay(100);
  SoundBufferCurrentlyPlaying = SoundBuffer0;
  SoundBuffer                 = SoundBuffer1;

  for (int i=0; i<DMABUFFERLENGTH; i++) DmaBuffer[i] = 0xffffffff; // prevents OLED reset
  
  startDMA();

  // ** just check the write speed **
  uint32_t startTime = micros();
  for (n = 0; n < 1000; n++) writeDac(0xff); // dummy write silence
  uint32_t stopTime = micros();

  uint32_t DacSamplingFrequency;
  //Serial.print("DAC write time for 1000 samples in us: "); Serial.println(stopTime - startTime);
  DacSamplingFrequency = 1000000UL * 1000 / (stopTime - startTime);
  //Serial.print("maximum possible DAC sampling frequency [Hz]: "); Serial.println(DacSamplingFrequency);

  // *********************************

  startTimer();
}

volatile uint32_t Oscillator1_Frequency_Hz=440;

void updateSound()
{
  for(uint32_t n=0;n<SOUNDBUFFERLENGTH;n++)
  { 
    SoundBuffer[n] = pt3_play();
  }

}

void loop()
{
  SoundBuffer = SoundBufferCurrentlyPlaying; 
  while(SoundBuffer == SoundBufferCurrentlyPlaying); // wait until buffer is emtpy
  updateSound();
}
