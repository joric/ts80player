// defines pt3_init() and pt3_play()
// you don't need to include other files

#if 0
#include "fatal.pt3.h"
#define music_data      fatal_pt3
#define music_data_size fatal_pt3_len
#endif

#if 1
#include "rage.pt3.h"
#define music_data      rage_pt3
#define music_data_size rage_pt3_len
#endif

volatile uint32_t t = 0;

#define AY_CLOCK      1773400         //pitch
#define SAMPLE_RATE   44010           //quality of the sound, i2s DAC can't handle more than 44100 by some reason (not even 48000)
#define FRAME_RATE    50              //speed

#include "ay_emu.h"

int interruptCnt;

struct PT3_Channel_Parameters
{
  unsigned short Address_In_Pattern, OrnamentPointer, SamplePointer, Ton;
  unsigned char Loop_Ornament_Position, Ornament_Length, Position_In_Ornament, Loop_Sample_Position, Sample_Length, Position_In_Sample, Volume, Number_Of_Notes_To_Skip, Note, Slide_To_Note, Amplitude;
  bool Envelope_Enabled, Enabled, SimpleGliss;
  short Current_Amplitude_Sliding, Current_Noise_Sliding, Current_Envelope_Sliding, Ton_Slide_Count, Current_OnOff, OnOff_Delay, OffOn_Delay, Ton_Slide_Delay, Current_Ton_Sliding, Ton_Accumulator, Ton_Slide_Step, Ton_Delta;
  signed char Note_Skip_Counter;
};

struct PT3_Parameters
{
  unsigned char Env_Base_lo;
  unsigned char Env_Base_hi;
  short Cur_Env_Slide, Env_Slide_Add;
  signed char Cur_Env_Delay, Env_Delay;
  unsigned char Noise_Base, Delay, AddToNoise, DelayCounter, CurrentPosition;
  int Version;
};

struct PT3_SongInfo
{
  PT3_Parameters PT3;
  PT3_Channel_Parameters PT3_A, PT3_B, PT3_C;
};

struct AYSongInfo
{
  unsigned char* module;
  unsigned char* module1;
  int module_len;
  PT3_SongInfo data;
  PT3_SongInfo data1;
  bool is_ts;

  AYChipStruct chip0;
  AYChipStruct chip1;
};

struct AYSongInfo AYInfo;

void ay_resetay(AYSongInfo* info, int chipnum)
{
  if (!chipnum) ay_init(&info->chip0); else ay_init(&info->chip1);
}

void ay_writeay(AYSongInfo* info, int reg, int val, int chipnum)
{
  if (!chipnum) ay_out(&info->chip0, reg, val); else ay_out(&info->chip1, reg, val);
}

#include "PT3Play.h"


uint32_t emulate_sample(void)
{
  uint32_t out_l, out_r;

  if (interruptCnt++ >= (SAMPLE_RATE / FRAME_RATE))
  {
    PT3_Play_Chip(AYInfo, 0);
    interruptCnt = 0;
  }

  ay_tick(&AYInfo.chip0, (AY_CLOCK / SAMPLE_RATE / 8));

  out_l = AYInfo.chip0.out[0] + AYInfo.chip0.out[1] / 2;
  out_r = AYInfo.chip0.out[2] + AYInfo.chip0.out[1] / 2;

  if (AYInfo.is_ts)
  {
    ay_tick(&AYInfo.chip1, (AY_CLOCK / SAMPLE_RATE / 8));

    out_l += AYInfo.chip0.out[0] + AYInfo.chip0.out[1] / 2;
    out_r += AYInfo.chip0.out[2] + AYInfo.chip0.out[1] / 2;
  }

  if (out_l > 32767) out_l = 32767;
  if (out_r > 32767) out_r = 32767;

  return out_l | (out_r << 16);
}


void pt3_init(){
  memset(&AYInfo, 0, sizeof(AYInfo));

  ay_init(&AYInfo.chip0);
  ay_init(&AYInfo.chip1);

  AYInfo.module = music_data;
  AYInfo.module_len = music_data_size;

  PT3_Init(AYInfo);
}

uint32_t pt3_play() {
    uint32_t out = emulate_sample();
    uint32_t amp = ((out & 0xff00) >> 8) + ((out & 0xff000000) >> 24); //convert to 8-bit mono
	return amp;
}

