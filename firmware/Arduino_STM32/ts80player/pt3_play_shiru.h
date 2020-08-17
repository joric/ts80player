// defines pt3_init() and pt3_play()
// you don't need to include other files
// ported from https://github.com/ESPboy-edu/ESPboy_PT3Play
// C++ version by Shiru
// Plain C version by Joric, 2019

#include "music_data.h"

#include <stdint.h>
#include <string.h>				// memset
#include <stdio.h>				// file
#include <stdbool.h>			// bool

volatile uint32_t t = 0;

#define AY_CLOCK      1773400	//pitch
#define SAMPLE_RATE   44100		//quality of the sound, i2s DAC can't handle more than 44100 by some reason (not even 48000)
#define FRAME_RATE    50		//speed

#ifdef PWM_FREQ
#undef SAMPLE_RATE
#define SAMPLE_RATE PWM_FREQ
#endif

typedef struct {
	int count;
	int state;
} toneStruct;

typedef struct {
	int count;
	int reg;
	int qcc;
	int state;
} noiseStruct;

typedef struct {
	int count;
	int dac;
	int up;
} envStruct;

typedef struct {
	toneStruct tone[3];
	noiseStruct noise;
	envStruct env;
	int reg[16];
	int dac[3];
	int out[3];
	int freqDiv;
} AYChipStruct;

//

#define VDIV	3

int volTab[16] = {
	0 / VDIV,
	836 / VDIV,
	1212 / VDIV,
	1773 / VDIV,
	2619 / VDIV,
	3875 / VDIV,
	5397 / VDIV,
	8823 / VDIV,
	10392 / VDIV,
	16706 / VDIV,
	23339 / VDIV,
	29292 / VDIV,
	36969 / VDIV,
	46421 / VDIV,
	55195 / VDIV,
	65535 / VDIV
};

void ay_init(AYChipStruct * ay) {
	memset(ay, 0, sizeof(AYChipStruct));
	ay->noise.reg = 0x0ffff;
	ay->noise.qcc = 0;
	ay->noise.state = 0;
}

void ay_out(AYChipStruct * ay, int reg, int value) {
	if (reg > 13)
		return;
	//
	//
	switch (reg) {
		case 1:
		case 3:
		case 5:
			value &= 15;
			break;
		case 8:
		case 9:
		case 10:
		case 6:
			value &= 31;
			break;
		case 13:
			value &= 15;
			ay->env.count = 0;
			if (value & 2) {
				ay->env.dac = 0;
				ay->env.up = 1;
			} else {
				ay->env.dac = 15;
				ay->env.up = 0;
			}
			break;
	}

	ay->reg[reg] = value;
}



inline void ay_tick(AYChipStruct * ay, int ticks) {
	int noise_di;
	int i, ta, tb, tc, na, nb, nc;

	ay->out[0] = 0;
	ay->out[1] = 0;
	ay->out[2] = 0;

	for (i = 0; i < ticks; ++i) {
		//
		ay->freqDiv ^= 1;

		//

		if (ay->tone[0].count >= (ay->reg[0] | (ay->reg[1] << 8))) {
			ay->tone[0].count = 0;
			ay->tone[0].state ^= 1;
		}
		if (ay->tone[1].count >= (ay->reg[2] | (ay->reg[3] << 8))) {
			ay->tone[1].count = 0;
			ay->tone[1].state ^= 1;
		}
		if (ay->tone[2].count >= (ay->reg[4] | (ay->reg[5] << 8))) {
			ay->tone[2].count = 0;
			ay->tone[2].state ^= 1;
		}

		ay->tone[0].count++;
		ay->tone[1].count++;
		ay->tone[2].count++;


		if (ay->freqDiv) {

			// (C)HackerKAY)

			if (ay->noise.count == 0) {
				noise_di = (ay->noise.qcc ^ ((ay->noise.reg >> 13) & 1)) ^ 1;
				ay->noise.qcc = (ay->noise.reg >> 15) & 1;
				ay->noise.state = ay->noise.qcc;
				ay->noise.reg = (ay->noise.reg << 1) | noise_di;
			}

			ay->noise.count = (ay->noise.count + 1) & 31;
			if (ay->noise.count >= ay->reg[6])
				ay->noise.count = 0;

			//

			if (ay->env.count == 0) {
				switch (ay->reg[13]) {
					case 0:
					case 1:
					case 2:
					case 3:
					case 9:
						if (ay->env.dac > 0)
							ay->env.dac--;
						break;
					case 4:
					case 5:
					case 6:
					case 7:
					case 15:
						if (ay->env.up) {
							ay->env.dac++;
							if (ay->env.dac > 15) {
								ay->env.dac = 0;
								ay->env.up = 0;
							}
						}
						break;

					case 8:
						ay->env.dac--;
						if (ay->env.dac < 0)
							ay->env.dac = 15;
						break;

					case 10:
					case 14:
						if (!ay->env.up) {
							ay->env.dac--;
							if (ay->env.dac < 0) {
								ay->env.dac = 0;
								ay->env.up = 1;
							}
						} else {
							ay->env.dac++;
							if (ay->env.dac > 15) {
								ay->env.dac = 15;
								ay->env.up = 0;
							}

						}
						break;

					case 11:
						if (!ay->env.up) {
							ay->env.dac--;
							if (ay->env.dac < 0) {
								ay->env.dac = 15;
								ay->env.up = 1;
							}
						}
						break;

					case 12:
						ay->env.dac++;
						if (ay->env.dac > 15)
							ay->env.dac = 0;
						break;

					case 13:
						if (ay->env.dac < 15)
							ay->env.dac++;
						break;

				}
			}

			ay->env.count++;
			if (ay->env.count >= (ay->reg[11] | (ay->reg[12] << 8)))
				ay->env.count = 0;

		}

		ta = ay->tone[0].state | ((ay->reg[7] >> 0) & 1);
		tb = ay->tone[1].state | ((ay->reg[7] >> 1) & 1);
		tc = ay->tone[2].state | ((ay->reg[7] >> 2) & 1);
		na = ay->noise.state | ((ay->reg[7] >> 3) & 1);
		nb = ay->noise.state | ((ay->reg[7] >> 4) & 1);
		nc = ay->noise.state | ((ay->reg[7] >> 5) & 1);

		if (ay->reg[8] & 16) {
			ay->dac[0] = ay->env.dac;
		} else {
			if (ta & na)
				ay->dac[0] = ay->reg[8];
			else
				ay->dac[0] = 0;
		}

		if (ay->reg[9] & 16) {
			ay->dac[1] = ay->env.dac;
		} else {
			if (tb & nb)
				ay->dac[1] = ay->reg[9];
			else
				ay->dac[1] = 0;
		}

		if (ay->reg[10] & 16) {
			ay->dac[2] = ay->env.dac;
		} else {
			if (tc & nc)
				ay->dac[2] = ay->reg[10];
			else
				ay->dac[2] = 0;
		}

		ay->out[0] += volTab[ay->dac[0]];
		ay->out[1] += volTab[ay->dac[1]];
		ay->out[2] += volTab[ay->dac[2]];
	}

	ay->out[0] /= ticks;
	ay->out[1] /= ticks;
	ay->out[2] /= ticks;
}


int interruptCnt;

typedef struct {
	unsigned short Address_In_Pattern, OrnamentPointer, SamplePointer, Ton;
	unsigned char Loop_Ornament_Position, Ornament_Length, Position_In_Ornament, Loop_Sample_Position, Sample_Length, Position_In_Sample, Volume, Number_Of_Notes_To_Skip, Note, Slide_To_Note, Amplitude;
	bool Envelope_Enabled, Enabled, SimpleGliss;
	short Current_Amplitude_Sliding, Current_Noise_Sliding, Current_Envelope_Sliding, Ton_Slide_Count, Current_OnOff, OnOff_Delay, OffOn_Delay, Ton_Slide_Delay, Current_Ton_Sliding, Ton_Accumulator, Ton_Slide_Step, Ton_Delta;
	signed char Note_Skip_Counter;
} PT3_Channel_Parameters;

typedef struct {
	unsigned char Env_Base_lo;
	unsigned char Env_Base_hi;
	short Cur_Env_Slide, Env_Slide_Add;
	signed char Cur_Env_Delay, Env_Delay;
	unsigned char Noise_Base, Delay, AddToNoise, DelayCounter, CurrentPosition;
	int Version;
} PT3_Parameters;

typedef struct {
	PT3_Parameters PT3;
	PT3_Channel_Parameters PT3_A, PT3_B, PT3_C;
} PT3_SongInfo;

typedef struct {
	unsigned char *module;
	unsigned char *module1;
	int module_len;
	PT3_SongInfo data;
	PT3_SongInfo data1;
	bool is_ts;
	AYChipStruct chip0;
	AYChipStruct chip1;
} AYSongInfo;

AYSongInfo AYInfo;

void ay_resetay(AYSongInfo * info, int chipnum) {
	if (!chipnum)
		ay_init(&info->chip0);
	else
		ay_init(&info->chip1);
}

void ay_writeay(AYSongInfo * info, int reg, int val, int chipnum) {
	if (!chipnum)
		ay_out(&info->chip0, reg, val);
	else
		ay_out(&info->chip1, reg, val);
}

/// >8 -- PT3Play.h begin ---
//Pro tracker 3.x player was written by S.V. Bulba.
//modified code from libayfly

#include <stdlib.h>				//abs

enum {
	AY_CHNL_A_FINE = 0,
	AY_CHNL_A_COARSE,
	AY_CHNL_B_FINE,
	AY_CHNL_B_COARSE,
	AY_CHNL_C_FINE,
	AY_CHNL_C_COARSE,
	AY_NOISE_PERIOD,
	AY_MIXER,
	AY_CHNL_A_VOL,
	AY_CHNL_B_VOL,
	AY_CHNL_C_VOL,
	AY_ENV_FINE,
	AY_ENV_COARSE,
	AY_ENV_SHAPE,
	AY_GPIO_A,
	AY_GPIO_B
};

int ay_sys_getword(unsigned char *data) {
	return data[0] + data[1] * 256;
}



//Table #0 of Pro Tracker 3.3x - 3.4r
const unsigned short PT3NoteTable_PT_33_34r[] = { 0x0C21, 0x0B73, 0x0ACE, 0x0A33, 0x09A0, 0x0916, 0x0893, 0x0818, 0x07A4, 0x0736, 0x06CE, 0x066D, 0x0610, 0x05B9, 0x0567, 0x0519, 0x04D0, 0x048B, 0x0449, 0x040C, 0x03D2, 0x039B, 0x0367, 0x0336, 0x0308, 0x02DC, 0x02B3, 0x028C, 0x0268, 0x0245, 0x0224, 0x0206, 0x01E9, 0x01CD, 0x01B3, 0x019B, 0x0184, 0x016E, 0x0159, 0x0146, 0x0134, 0x0122, 0x0112, 0x0103, 0x00F4, 0x00E6, 0x00D9, 0x00CD, 0x00C2, 0x00B7, 0x00AC, 0x00A3, 0x009A, 0x0091, 0x0089, 0x0081, 0x007A, 0x0073, 0x006C, 0x0066, 0x0061, 0x005B, 0x0056, 0x0051, 0x004D, 0x0048, 0x0044, 0x0040, 0x003D, 0x0039, 0x0036, 0x0033, 0x0030, 0x002D, 0x002B, 0x0028, 0x0026, 0x0024, 0x0022, 0x0020, 0x001E, 0x001C, 0x001B, 0x0019, 0x0018, 0x0016, 0x0015, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000F, 0x000E, 0x000D, 0x000C };

//{Table #0 of Pro Tracker 3.4x - 3.5x}
const unsigned short PT3NoteTable_PT_34_35[] = { 0x0C22, 0x0B73, 0x0ACF, 0x0A33, 0x09A1, 0x0917, 0x0894, 0x0819, 0x07A4, 0x0737, 0x06CF, 0x066D, 0x0611, 0x05BA, 0x0567, 0x051A, 0x04D0, 0x048B, 0x044A, 0x040C, 0x03D2, 0x039B, 0x0367, 0x0337, 0x0308, 0x02DD, 0x02B4, 0x028D, 0x0268, 0x0246, 0x0225, 0x0206, 0x01E9, 0x01CE, 0x01B4, 0x019B, 0x0184, 0x016E, 0x015A, 0x0146, 0x0134, 0x0123, 0x0112, 0x0103, 0x00F5, 0x00E7, 0x00DA, 0x00CE, 0x00C2, 0x00B7, 0x00AD, 0x00A3, 0x009A, 0x0091, 0x0089, 0x0082, 0x007A, 0x0073, 0x006D, 0x0067, 0x0061, 0x005C, 0x0056, 0x0052, 0x004D, 0x0049, 0x0045, 0x0041, 0x003D, 0x003A, 0x0036, 0x0033, 0x0031, 0x002E, 0x002B, 0x0029, 0x0027, 0x0024, 0x0022, 0x0020, 0x001F, 0x001D, 0x001B, 0x001A, 0x0018, 0x0017, 0x0016, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000F, 0x000E, 0x000D, 0x000C };

//{Table #1 of Pro Tracker 3.3x - 3.5x)}
const unsigned short PT3NoteTable_ST[] = { 0x0EF8, 0x0E10, 0x0D60, 0x0C80, 0x0BD8, 0x0B28, 0x0A88, 0x09F0, 0x0960, 0x08E0, 0x0858, 0x07E0, 0x077C, 0x0708, 0x06B0, 0x0640, 0x05EC, 0x0594, 0x0544, 0x04F8, 0x04B0, 0x0470, 0x042C, 0x03FD, 0x03BE, 0x0384, 0x0358, 0x0320, 0x02F6, 0x02CA, 0x02A2, 0x027C, 0x0258, 0x0238, 0x0216, 0x01F8, 0x01DF, 0x01C2, 0x01AC, 0x0190, 0x017B, 0x0165, 0x0151, 0x013E, 0x012C, 0x011C, 0x010A, 0x00FC, 0x00EF, 0x00E1, 0x00D6, 0x00C8, 0x00BD, 0x00B2, 0x00A8, 0x009F, 0x0096, 0x008E, 0x0085, 0x007E, 0x0077, 0x0070, 0x006B, 0x0064, 0x005E, 0x0059, 0x0054, 0x004F, 0x004B, 0x0047, 0x0042, 0x003F, 0x003B, 0x0038, 0x0035, 0x0032, 0x002F, 0x002C, 0x002A, 0x0027, 0x0025, 0x0023, 0x0021, 0x001F, 0x001D, 0x001C, 0x001A, 0x0019, 0x0017, 0x0016, 0x0015, 0x0013, 0x0012, 0x0011, 0x0010, 0x000F };

//{Table #2 of Pro Tracker 3.4r}
const unsigned short PT3NoteTable_ASM_34r[] = { 0x0D3E, 0x0C80, 0x0BCC, 0x0B22, 0x0A82, 0x09EC, 0x095C, 0x08D6, 0x0858, 0x07E0, 0x076E, 0x0704, 0x069F, 0x0640, 0x05E6, 0x0591, 0x0541, 0x04F6, 0x04AE, 0x046B, 0x042C, 0x03F0, 0x03B7, 0x0382, 0x034F, 0x0320, 0x02F3, 0x02C8, 0x02A1, 0x027B, 0x0257, 0x0236, 0x0216, 0x01F8, 0x01DC, 0x01C1, 0x01A8, 0x0190, 0x0179, 0x0164, 0x0150, 0x013D, 0x012C, 0x011B, 0x010B, 0x00FC, 0x00EE, 0x00E0, 0x00D4, 0x00C8, 0x00BD, 0x00B2, 0x00A8, 0x009F, 0x0096, 0x008D, 0x0085, 0x007E, 0x0077, 0x0070, 0x006A, 0x0064, 0x005E, 0x0059, 0x0054, 0x0050, 0x004B, 0x0047, 0x0043, 0x003F, 0x003C, 0x0038, 0x0035, 0x0032, 0x002F, 0x002D, 0x002A, 0x0028, 0x0026, 0x0024, 0x0022, 0x0020, 0x001E, 0x001D, 0x001B, 0x001A, 0x0019, 0x0018, 0x0015, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000F, 0x000E };

//{Table #2 of Pro Tracker 3.4x - 3.5x}
const unsigned short PT3NoteTable_ASM_34_35[] = { 0x0D10, 0x0C55, 0x0BA4, 0x0AFC, 0x0A5F, 0x09CA, 0x093D, 0x08B8, 0x083B, 0x07C5, 0x0755, 0x06EC, 0x0688, 0x062A, 0x05D2, 0x057E, 0x052F, 0x04E5, 0x049E, 0x045C, 0x041D, 0x03E2, 0x03AB, 0x0376, 0x0344, 0x0315, 0x02E9, 0x02BF, 0x0298, 0x0272, 0x024F, 0x022E, 0x020F, 0x01F1, 0x01D5, 0x01BB, 0x01A2, 0x018B, 0x0174, 0x0160, 0x014C, 0x0139, 0x0128, 0x0117, 0x0107, 0x00F9, 0x00EB, 0x00DD, 0x00D1, 0x00C5, 0x00BA, 0x00B0, 0x00A6, 0x009D, 0x0094, 0x008C, 0x0084, 0x007C, 0x0075, 0x006F, 0x0069, 0x0063, 0x005D, 0x0058, 0x0053, 0x004E, 0x004A, 0x0046, 0x0042, 0x003E, 0x003B, 0x0037, 0x0034, 0x0031, 0x002F, 0x002C, 0x0029, 0x0027, 0x0025, 0x0023, 0x0021, 0x001F, 0x001D, 0x001C, 0x001A, 0x0019, 0x0017, 0x0016, 0x0015, 0x0014, 0x0012, 0x0011, 0x0010, 0x000F, 0x000E, 0x000D };

//{Table #3 of Pro Tracker 3.4r}
const unsigned short PT3NoteTable_REAL_34r[] = { 0x0CDA, 0x0C22, 0x0B73, 0x0ACF, 0x0A33, 0x09A1, 0x0917, 0x0894, 0x0819, 0x07A4, 0x0737, 0x06CF, 0x066D, 0x0611, 0x05BA, 0x0567, 0x051A, 0x04D0, 0x048B, 0x044A, 0x040C, 0x03D2, 0x039B, 0x0367, 0x0337, 0x0308, 0x02DD, 0x02B4, 0x028D, 0x0268, 0x0246, 0x0225, 0x0206, 0x01E9, 0x01CE, 0x01B4, 0x019B, 0x0184, 0x016E, 0x015A, 0x0146, 0x0134, 0x0123, 0x0113, 0x0103, 0x00F5, 0x00E7, 0x00DA, 0x00CE, 0x00C2, 0x00B7, 0x00AD, 0x00A3, 0x009A, 0x0091, 0x0089, 0x0082, 0x007A, 0x0073, 0x006D, 0x0067, 0x0061, 0x005C, 0x0056, 0x0052, 0x004D, 0x0049, 0x0045, 0x0041, 0x003D, 0x003A, 0x0036, 0x0033, 0x0031, 0x002E, 0x002B, 0x0029, 0x0027, 0x0024, 0x0022, 0x0020, 0x001F, 0x001D, 0x001B, 0x001A, 0x0018, 0x0017, 0x0016, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000F, 0x000E, 0x000D };

//{Table #3 of Pro Tracker 3.4x - 3.5x}
const unsigned short PT3NoteTable_REAL_34_35[] = { 0x0CDA, 0x0C22, 0x0B73, 0x0ACF, 0x0A33, 0x09A1, 0x0917, 0x0894, 0x0819, 0x07A4, 0x0737, 0x06CF, 0x066D, 0x0611, 0x05BA, 0x0567, 0x051A, 0x04D0, 0x048B, 0x044A, 0x040C, 0x03D2, 0x039B, 0x0367, 0x0337, 0x0308, 0x02DD, 0x02B4, 0x028D, 0x0268, 0x0246, 0x0225, 0x0206, 0x01E9, 0x01CE, 0x01B4, 0x019B, 0x0184, 0x016E, 0x015A, 0x0146, 0x0134, 0x0123, 0x0112, 0x0103, 0x00F5, 0x00E7, 0x00DA, 0x00CE, 0x00C2, 0x00B7, 0x00AD, 0x00A3, 0x009A, 0x0091, 0x0089, 0x0082, 0x007A, 0x0073, 0x006D, 0x0067, 0x0061, 0x005C, 0x0056, 0x0052, 0x004D, 0x0049, 0x0045, 0x0041, 0x003D, 0x003A, 0x0036, 0x0033, 0x0031, 0x002E, 0x002B, 0x0029, 0x0027, 0x0024, 0x0022, 0x0020, 0x001F, 0x001D, 0x001B, 0x001A, 0x0018, 0x0017, 0x0016, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000F, 0x000E, 0x000D };

//{Volume table of Pro Tracker 3.3x - 3.4x}
const unsigned char PT3VolumeTable_33_34[16][16] = {
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02},
	{0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03},
	{0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04},
	{0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05},
	{0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06},
	{0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07},
	{0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08},
	{0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09},
	{0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0A},
	{0x00, 0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x09, 0x0A, 0x0B},
	{0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0A, 0x0B, 0x0C},
	{0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D},
	{0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E},
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F}
};

//{Volume table of Pro Tracker 3.5x}
const unsigned char PT3VolumeTable_35[16][16] = {
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
	{0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02},
	{0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03},
	{0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04},
	{0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05},
	{0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06},
	{0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07},
	{0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08},
	{0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09},
	{0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x09, 0x0A},
	{0x00, 0x01, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0A, 0x0B},
	{0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0A, 0x0B, 0x0C},
	{0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0A, 0x0B, 0x0C, 0x0D},
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E},
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F}
};

typedef struct {
	signed char PT3_MusicName[0x63];
	unsigned char PT3_TonTableId;
	unsigned char PT3_Delay;
	unsigned char PT3_NumberOfPositions;
	unsigned char PT3_LoopPosition;
	unsigned char PT3_PatternsPointer0, PT3_PatternsPointer1;
	unsigned char PT3_SamplesPointers0[32 * 2];
	unsigned char PT3_OrnamentsPointers0[16 * 2];
	unsigned char PT3_PositionList[128 * 2];
} PT3_File;

#define PT3_PatternsPointer (header->PT3_PatternsPointer0 | (header->PT3_PatternsPointer1 << 8))
#define PT3_SamplesPointers(x) (header->PT3_SamplesPointers0 [(x) * 2] | (header->PT3_SamplesPointers0 [(x) * 2 + 1] << 8))
#define PT3_OrnamentsPointers(x) (header->PT3_OrnamentsPointers0 [(x) * 2] | (header->PT3_OrnamentsPointers0 [(x) * 2 + 1] << 8))

#define PT3_A ((PT3_SongInfo *)data)->PT3_A
#define PT3_B ((PT3_SongInfo *)data)->PT3_B
#define PT3_C ((PT3_SongInfo *)data)->PT3_C
#define PT3 ((PT3_SongInfo *)data)->PT3

#define GET_COMMON_VARS(x) \
    unsigned char *module; (void)module;\
    PT3_File *header; (void)header;\
    PT3_SongInfo *data;\
    if(!info->is_ts || x == 0)\
    {\
        module = info->module;\
        header = (PT3_File *)info->module;\
        data = &info->data;\
    }\
    else\
    {\
        module = info->module1;\
        header = (PT3_File *)info->module1;\
        data = &info->data1;\
    }


unsigned char *PT3_FindSig(unsigned char *buffer, long length) {
	char sig[] = { 0x50, 0x72, 0x6f, 0x54, 0x72, 0x61, 0x63, 0x6b, 0x65, 0x72, 0x20, 0x33, 0x2e };
	char sig1[] = { 0x56, 0x6f, 0x72, 0x74, 0x65, 0x78, 0x20, 0x54, 0x72, 0x61, 0x63, 0x6b, 0x65, 0x72, 0x20, 0x49, 0x49 };
	if (length < sizeof(sig) || length < sizeof(sig1))
		return 0;
	unsigned char *ptr = buffer;
	unsigned long remaining = length - sizeof(sig);
	while (remaining > sizeof(sig)) {
		if (!memcmp(ptr, sig, sizeof(sig)) || !memcmp(ptr, sig1, sizeof(sig1)))	//found
		{
			return ptr;
		}
		ptr++;
		remaining--;
	}
	return 0;
}

void PT3_Init(AYSongInfo * info) {
	int i;
	unsigned char b;
	unsigned char *module = info->module;
	PT3_File *header = (PT3_File *) module;

	memset(&info->data, 0, sizeof(PT3_SongInfo));

	int version = 6;

	if ((header->PT3_MusicName[13] >= '0') && (header->PT3_MusicName[13] <= '9')) {
		version = header->PT3_MusicName[13] - 0x30;
	}
	unsigned char *ptr = PT3_FindSig(module + 0x63, info->module_len - 0x63);
	if ((unsigned long)ptr > 0) {
		info->is_ts = true;
		info->module1 = ptr;
		memset(&info->data1, 0, sizeof(PT3_SongInfo));
	}

	void *data = (void *)&info->data;
	module = info->module;

	for (unsigned long y = 0; y < 2; y++) {
		i = header->PT3_PositionList[0];
		b = header->PT3_MusicName[0x62];
		if (b != 0x20) {
			i = b * 3 - 3 - i;
		}
		PT3.Version = version;
		PT3.DelayCounter = 1;
		PT3.Delay = header->PT3_Delay;
		PT3_A.Address_In_Pattern = ay_sys_getword(&module[PT3_PatternsPointer + i * 2]);
		PT3_B.Address_In_Pattern = ay_sys_getword(&module[PT3_PatternsPointer + i * 2 + 2]);
		PT3_C.Address_In_Pattern = ay_sys_getword(&module[PT3_PatternsPointer + i * 2 + 4]);

		PT3_A.OrnamentPointer = PT3_OrnamentsPointers(0);
		PT3_A.Loop_Ornament_Position = module[PT3_A.OrnamentPointer];
		PT3_A.OrnamentPointer++;
		PT3_A.Ornament_Length = module[PT3_A.OrnamentPointer];
		PT3_A.OrnamentPointer++;
		PT3_A.SamplePointer = PT3_SamplesPointers(1);
		PT3_A.Loop_Sample_Position = module[PT3_A.SamplePointer];
		PT3_A.SamplePointer++;
		PT3_A.Sample_Length = module[PT3_A.SamplePointer];
		PT3_A.SamplePointer++;
		PT3_A.Volume = 15;
		PT3_A.Note_Skip_Counter = 1;

		PT3_B.OrnamentPointer = PT3_A.OrnamentPointer;
		PT3_B.Loop_Ornament_Position = PT3_A.Loop_Ornament_Position;
		PT3_B.Ornament_Length = PT3_A.Ornament_Length;
		PT3_B.SamplePointer = PT3_A.SamplePointer;
		PT3_B.Loop_Sample_Position = PT3_A.Loop_Sample_Position;
		PT3_B.Sample_Length = PT3_A.Sample_Length;
		PT3_B.Volume = 15;
		PT3_B.Note_Skip_Counter = 1;

		PT3_C.OrnamentPointer = PT3_A.OrnamentPointer;
		PT3_C.Loop_Ornament_Position = PT3_A.Loop_Ornament_Position;
		PT3_C.Ornament_Length = PT3_A.Ornament_Length;
		PT3_C.SamplePointer = PT3_A.SamplePointer;
		PT3_C.Loop_Sample_Position = PT3_A.Loop_Sample_Position;
		PT3_C.Sample_Length = PT3_A.Sample_Length;
		PT3_C.Volume = 15;
		PT3_C.Note_Skip_Counter = 1;
		if (!info->is_ts)
			break;
		data = (void *)&info->data1;
		module = info->module1;
		header = (PT3_File *) module;
	}

	ay_resetay(info, 0);
	ay_resetay(info, 1);
}

int PT3_GetNoteFreq(AYSongInfo * info, unsigned char j, unsigned long chip_num) {
	GET_COMMON_VARS(chip_num);
	switch (header->PT3_TonTableId) {
		case 0:
			if (PT3.Version <= 3)
				return PT3NoteTable_PT_33_34r[j];
			else
				return PT3NoteTable_PT_34_35[j];
		case 1:
			return PT3NoteTable_ST[j];
		case 2:
			if (PT3.Version <= 3)
				return PT3NoteTable_ASM_34r[j];
			else
				return PT3NoteTable_ASM_34_35[j];
		default:
			if (PT3.Version <= 3)
				return PT3NoteTable_REAL_34r[j];
			else
				return PT3NoteTable_REAL_34_35[j];
	}
}

void PT3_PatternIntterpreter(AYSongInfo * info, PT3_Channel_Parameters * chan, unsigned long chip_num) {
	GET_COMMON_VARS(chip_num);
	bool quit;
	unsigned char flag9, flag8, flag5, flag4, flag3, flag2, flag1;
	unsigned char counter;
	int prnote, prsliding;
	prnote = chan->Note;
	prsliding = chan->Current_Ton_Sliding;
	quit = false;
	counter = 0;
	flag9 = flag8 = flag5 = flag4 = flag3 = flag2 = flag1 = 0;
	do {
		unsigned char val = module[chan->Address_In_Pattern];
		if (val >= 0xf0) {
			chan->OrnamentPointer = PT3_OrnamentsPointers((val - 0xf0));
			chan->Loop_Ornament_Position = module[chan->OrnamentPointer];
			chan->OrnamentPointer++;
			chan->Ornament_Length = module[chan->OrnamentPointer];
			chan->OrnamentPointer++;
			chan->Address_In_Pattern++;
			chan->SamplePointer = PT3_SamplesPointers((module[chan->Address_In_Pattern] / 2));
			chan->Loop_Sample_Position = module[chan->SamplePointer];
			chan->SamplePointer++;
			chan->Sample_Length = module[chan->SamplePointer];
			chan->SamplePointer++;
			chan->Envelope_Enabled = false;
			chan->Position_In_Ornament = 0;
		} else if (val >= 0xd1 && val <= 0xef) {
			chan->SamplePointer = PT3_SamplesPointers((val - 0xd0));
			chan->Loop_Sample_Position = module[chan->SamplePointer];
			chan->SamplePointer++;
			chan->Sample_Length = module[chan->SamplePointer];
			chan->SamplePointer++;
		} else if (val == 0xd0) {
			quit = true;
		} else if (val >= 0xc1 && val <= 0xcf) {
			chan->Volume = val - 0xc0;
		} else if (val == 0xc0) {
			chan->Position_In_Sample = 0;
			chan->Current_Amplitude_Sliding = 0;
			chan->Current_Noise_Sliding = 0;
			chan->Current_Envelope_Sliding = 0;
			chan->Position_In_Ornament = 0;
			chan->Ton_Slide_Count = 0;
			chan->Current_Ton_Sliding = 0;
			chan->Ton_Accumulator = 0;
			chan->Current_OnOff = 0;
			chan->Enabled = false;
			quit = true;
		} else if (val >= 0xb2 && val <= 0xbf) {
			chan->Envelope_Enabled = true;
			ay_writeay(info, AY_ENV_SHAPE, val - 0xb1, chip_num);
			chan->Address_In_Pattern++;
			PT3.Env_Base_hi = module[chan->Address_In_Pattern];
			chan->Address_In_Pattern++;
			PT3.Env_Base_lo = module[chan->Address_In_Pattern];
			chan->Position_In_Ornament = 0;
			PT3.Cur_Env_Slide = 0;
			PT3.Cur_Env_Delay = 0;
		} else if (val == 0xb1) {
			chan->Address_In_Pattern++;
			chan->Number_Of_Notes_To_Skip = module[chan->Address_In_Pattern];
		} else if (val == 0xb0) {
			chan->Envelope_Enabled = false;
			chan->Position_In_Ornament = 0;
		} else if (val >= 0x50 && val <= 0xaf) {
			chan->Note = val - 0x50;
			chan->Position_In_Sample = 0;
			chan->Current_Amplitude_Sliding = 0;
			chan->Current_Noise_Sliding = 0;
			chan->Current_Envelope_Sliding = 0;
			chan->Position_In_Ornament = 0;
			chan->Ton_Slide_Count = 0;
			chan->Current_Ton_Sliding = 0;
			chan->Ton_Accumulator = 0;
			chan->Current_OnOff = 0;
			chan->Enabled = true;
			quit = true;
		} else if (val >= 0x40 && val <= 0x4f) {
			chan->OrnamentPointer = PT3_OrnamentsPointers((val - 0x40));
			chan->Loop_Ornament_Position = module[chan->OrnamentPointer];
			chan->OrnamentPointer++;
			chan->Ornament_Length = module[chan->OrnamentPointer];
			chan->OrnamentPointer++;
			chan->Position_In_Ornament = 0;
		} else if (val >= 0x20 && val <= 0x3f) {
			PT3.Noise_Base = val - 0x20;
		} else if (val >= 0x10 && val <= 0x1f) {
			if (val == 0x10)
				chan->Envelope_Enabled = false;
			else {
				ay_writeay(info, AY_ENV_SHAPE, val - 0x10, chip_num);
				chan->Address_In_Pattern++;
				PT3.Env_Base_hi = module[chan->Address_In_Pattern];
				chan->Address_In_Pattern++;
				PT3.Env_Base_lo = module[chan->Address_In_Pattern];
				chan->Envelope_Enabled = true;
				PT3.Cur_Env_Slide = 0;
				PT3.Cur_Env_Delay = 0;
			}
			chan->Address_In_Pattern++;
			chan->SamplePointer = PT3_SamplesPointers((module[chan->Address_In_Pattern] / 2));
			chan->Loop_Sample_Position = module[chan->SamplePointer];
			chan->SamplePointer++;
			chan->Sample_Length = module[chan->SamplePointer];
			chan->SamplePointer++;
			chan->Position_In_Ornament = 0;
		} else if (val == 0x9) {
			counter++;
			flag9 = counter;
		} else if (val == 0x8) {
			counter++;
			flag8 = counter;
		} else if (val == 0x5) {
			counter++;
			flag5 = counter;
		} else if (val == 0x4) {
			counter++;
			flag4 = counter;
		} else if (val == 0x3) {
			counter++;
			flag3 = counter;
		} else if (val == 0x2) {
			counter++;
			flag2 = counter;
		} else if (val == 0x1) {
			counter++;
			flag1 = counter;
		}
		chan->Address_In_Pattern++;
	}
	while (!quit);

	while (counter > 0) {
		if (counter == flag1) {
			chan->Ton_Slide_Delay = module[chan->Address_In_Pattern];
			chan->Ton_Slide_Count = chan->Ton_Slide_Delay;
			chan->Address_In_Pattern++;
			chan->Ton_Slide_Step = ay_sys_getword(&module[chan->Address_In_Pattern]);
			chan->Address_In_Pattern += 2;
			chan->SimpleGliss = true;
			chan->Current_OnOff = 0;
			if ((chan->Ton_Slide_Count == 0) && (PT3.Version >= 7))
				chan->Ton_Slide_Count++;
		} else if (counter == flag2) {
			chan->SimpleGliss = false;
			chan->Current_OnOff = 0;
			chan->Ton_Slide_Delay = module[chan->Address_In_Pattern];
			chan->Ton_Slide_Count = chan->Ton_Slide_Delay;
			chan->Address_In_Pattern += 3;
			chan->Ton_Slide_Step = abs((short)(ay_sys_getword(&module[chan->Address_In_Pattern])));
			chan->Address_In_Pattern += 2;
			chan->Ton_Delta = PT3_GetNoteFreq(info, chan->Note, chip_num) - PT3_GetNoteFreq(info, prnote, chip_num);
			chan->Slide_To_Note = chan->Note;
			chan->Note = prnote;
			if (PT3.Version >= 6)
				chan->Current_Ton_Sliding = prsliding;
			if ((chan->Ton_Delta - chan->Current_Ton_Sliding) < 0)
				chan->Ton_Slide_Step = -chan->Ton_Slide_Step;
		} else if (counter == flag3) {
			chan->Position_In_Sample = module[chan->Address_In_Pattern];
			chan->Address_In_Pattern++;
		} else if (counter == flag4) {
			chan->Position_In_Ornament = module[chan->Address_In_Pattern];
			chan->Address_In_Pattern++;
		} else if (counter == flag5) {
			chan->OnOff_Delay = module[chan->Address_In_Pattern];
			chan->Address_In_Pattern++;
			chan->OffOn_Delay = module[chan->Address_In_Pattern];
			chan->Current_OnOff = chan->OnOff_Delay;
			chan->Address_In_Pattern++;
			chan->Ton_Slide_Count = 0;
			chan->Current_Ton_Sliding = 0;
		} else if (counter == flag8) {
			PT3.Env_Delay = module[chan->Address_In_Pattern];
			PT3.Cur_Env_Delay = PT3.Env_Delay;
			chan->Address_In_Pattern++;
			PT3.Env_Slide_Add = ay_sys_getword(&module[chan->Address_In_Pattern]);
			chan->Address_In_Pattern += 2;
		} else if (counter == flag9) {
			PT3.Delay = module[chan->Address_In_Pattern];
			chan->Address_In_Pattern++;
		}
		counter--;
	}
	chan->Note_Skip_Counter = chan->Number_Of_Notes_To_Skip;
}

void PT3_ChangeRegisters(AYSongInfo * info, PT3_Channel_Parameters * chan, char *AddToEnv, unsigned char *TempMixer, unsigned long chip_num) {
	GET_COMMON_VARS(chip_num);
	unsigned char j, b1, b0;
	unsigned short w;
	if (chan->Enabled) {
		chan->Ton = ay_sys_getword(&module[chan->SamplePointer + chan->Position_In_Sample * 4 + 2]);
		chan->Ton += chan->Ton_Accumulator;
		b0 = module[chan->SamplePointer + chan->Position_In_Sample * 4];
		b1 = module[chan->SamplePointer + chan->Position_In_Sample * 4 + 1];
		if ((b1 & 0x40) != 0) {
			chan->Ton_Accumulator = chan->Ton;
		}
		j = chan->Note + module[chan->OrnamentPointer + chan->Position_In_Ornament];
		if ((signed char)(j) < 0)
			j = 0;
		else if (j > 95)
			j = 95;
		w = PT3_GetNoteFreq(info, j, chip_num);
		chan->Ton = (chan->Ton + chan->Current_Ton_Sliding + w) & 0xfff;
		if (chan->Ton_Slide_Count > 0) {
			chan->Ton_Slide_Count--;
			if (chan->Ton_Slide_Count == 0) {
				chan->Current_Ton_Sliding += chan->Ton_Slide_Step;
				chan->Ton_Slide_Count = chan->Ton_Slide_Delay;
				if (!chan->SimpleGliss) {
					if (((chan->Ton_Slide_Step < 0) && (chan->Current_Ton_Sliding <= chan->Ton_Delta)) || ((chan->Ton_Slide_Step >= 0) && (chan->Current_Ton_Sliding >= chan->Ton_Delta))) {
						chan->Note = chan->Slide_To_Note;
						chan->Ton_Slide_Count = 0;
						chan->Current_Ton_Sliding = 0;
					}
				}
			}
		}
		chan->Amplitude = b1 & 0xf;
		if ((b0 & 0x80) != 0) {
			if ((b0 & 0x40) != 0) {
				if (chan->Current_Amplitude_Sliding < 15)
					chan->Current_Amplitude_Sliding++;
			} else if (chan->Current_Amplitude_Sliding > -15) {
				chan->Current_Amplitude_Sliding--;
			}
		}
		chan->Amplitude += chan->Current_Amplitude_Sliding;
		if ((signed char)(chan->Amplitude) < 0)
			chan->Amplitude = 0;
		else if (chan->Amplitude > 15)
			chan->Amplitude = 15;
		if (PT3.Version <= 4)
			chan->Amplitude = PT3VolumeTable_33_34[chan->Volume][chan->Amplitude];
		else
			chan->Amplitude = PT3VolumeTable_35[chan->Volume][chan->Amplitude];
		if (((b0 & 1) == 0) && chan->Envelope_Enabled)
			chan->Amplitude = chan->Amplitude | 16;
		if ((b1 & 0x80) != 0) {
			if ((b0 & 0x20) != 0)
				j = ((b0 >> 1) | 0xf0) + chan->Current_Envelope_Sliding;
			else
				j = ((b0 >> 1) & 0xf) + chan->Current_Envelope_Sliding;
			if ((b1 & 0x20) != 0)
				chan->Current_Envelope_Sliding = j;
			*AddToEnv += j;
		} else {
			PT3.AddToNoise = (b0 >> 1) + chan->Current_Noise_Sliding;
			if ((b1 & 0x20) != 0)
				chan->Current_Noise_Sliding = PT3.AddToNoise;
		}
		*TempMixer = ((b1 >> 1) & 0x48) | *TempMixer;
		chan->Position_In_Sample++;
		if (chan->Position_In_Sample >= chan->Sample_Length)
			chan->Position_In_Sample = chan->Loop_Sample_Position;
		chan->Position_In_Ornament++;
		if (chan->Position_In_Ornament >= chan->Ornament_Length)
			chan->Position_In_Ornament = chan->Loop_Ornament_Position;
	} else
		chan->Amplitude = 0;
	*TempMixer = *TempMixer >> 1;
	if (chan->Current_OnOff > 0) {
		chan->Current_OnOff--;
		if (chan->Current_OnOff == 0) {
			chan->Enabled = !chan->Enabled;
			if (chan->Enabled)
				chan->Current_OnOff = chan->OnOff_Delay;
			else
				chan->Current_OnOff = chan->OffOn_Delay;
		}
	}
}

void PT3_Play_Chip(AYSongInfo * info, unsigned long chip_num) {
	GET_COMMON_VARS(chip_num);
	unsigned char TempMixer;
	char AddToEnv;

	PT3.DelayCounter--;
	if (PT3.DelayCounter == 0) {
		PT3_A.Note_Skip_Counter--;
		if (PT3_A.Note_Skip_Counter == 0) {
			if (module[PT3_A.Address_In_Pattern] == 0) {
				PT3.CurrentPosition++;
				if (PT3.CurrentPosition == header->PT3_NumberOfPositions)
					PT3.CurrentPosition = header->PT3_LoopPosition;
				PT3_A.Address_In_Pattern = ay_sys_getword(&module[PT3_PatternsPointer + header->PT3_PositionList[PT3.CurrentPosition] * 2]);
				PT3_B.Address_In_Pattern = ay_sys_getword(&module[PT3_PatternsPointer + header->PT3_PositionList[PT3.CurrentPosition] * 2 + 2]);
				PT3_C.Address_In_Pattern = ay_sys_getword(&module[PT3_PatternsPointer + header->PT3_PositionList[PT3.CurrentPosition] * 2 + 4]);
				PT3.Noise_Base = 0;
			}
			PT3_PatternIntterpreter(info, &PT3_A, chip_num);
		}
		PT3_B.Note_Skip_Counter--;
		if (PT3_B.Note_Skip_Counter == 0)
			PT3_PatternIntterpreter(info, &PT3_B, chip_num);
		PT3_C.Note_Skip_Counter--;
		if (PT3_C.Note_Skip_Counter == 0)
			PT3_PatternIntterpreter(info, &PT3_C, chip_num);
		PT3.DelayCounter = PT3.Delay;
	}

	AddToEnv = 0;
	TempMixer = 0;
	PT3_ChangeRegisters(info, &PT3_A, &AddToEnv, &TempMixer, chip_num);
	PT3_ChangeRegisters(info, &PT3_B, &AddToEnv, &TempMixer, chip_num);
	PT3_ChangeRegisters(info, &PT3_C, &AddToEnv, &TempMixer, chip_num);

	ay_writeay(info, AY_MIXER, TempMixer, chip_num);

	ay_writeay(info, AY_CHNL_A_FINE, PT3_A.Ton & 0xff, chip_num);
	ay_writeay(info, AY_CHNL_A_COARSE, (PT3_A.Ton >> 8) & 0xf, chip_num);
	ay_writeay(info, AY_CHNL_B_FINE, PT3_B.Ton & 0xff, chip_num);
	ay_writeay(info, AY_CHNL_B_COARSE, (PT3_B.Ton >> 8) & 0xf, chip_num);
	ay_writeay(info, AY_CHNL_C_FINE, PT3_C.Ton & 0xff, chip_num);
	ay_writeay(info, AY_CHNL_C_COARSE, (PT3_C.Ton >> 8) & 0xf, chip_num);
	ay_writeay(info, AY_CHNL_A_VOL, PT3_A.Amplitude, chip_num);
	ay_writeay(info, AY_CHNL_B_VOL, PT3_B.Amplitude, chip_num);
	ay_writeay(info, AY_CHNL_C_VOL, PT3_C.Amplitude, chip_num);

	ay_writeay(info, AY_NOISE_PERIOD, (PT3.Noise_Base + PT3.AddToNoise) & 31, chip_num);
	unsigned short cur_env = ay_sys_getword(&PT3.Env_Base_lo) + AddToEnv + PT3.Cur_Env_Slide;
	ay_writeay(info, AY_ENV_FINE, cur_env & 0xff, chip_num);
	ay_writeay(info, AY_ENV_COARSE, (cur_env >> 8) & 0xff, chip_num);

	if (PT3.Cur_Env_Delay > 0) {
		PT3.Cur_Env_Delay--;
		if (PT3.Cur_Env_Delay == 0) {
			PT3.Cur_Env_Delay = PT3.Env_Delay;
			PT3.Cur_Env_Slide += PT3.Env_Slide_Add;
		}
	}
}

void PT3_Play(AYSongInfo * info) {
	PT3_Play_Chip(info, 0);
	if (info->is_ts)
		PT3_Play_Chip(info, 1);
}

/* PT3Play.h end */

uint32_t emulate_sample(void) {
	uint32_t out_l, out_r;

	if (interruptCnt++ >= (SAMPLE_RATE / FRAME_RATE)) {
		PT3_Play(&AYInfo);
		interruptCnt = 0;
	}

	ay_tick(&AYInfo.chip0, (AY_CLOCK / SAMPLE_RATE / 8));

	out_l = AYInfo.chip0.out[0] + AYInfo.chip0.out[1] / 2;
	out_r = AYInfo.chip0.out[2] + AYInfo.chip0.out[1] / 2;

	if (AYInfo.is_ts) {
		ay_tick(&AYInfo.chip1, (AY_CLOCK / SAMPLE_RATE / 8));

		out_l += AYInfo.chip1.out[0] + AYInfo.chip1.out[1] / 2;
		out_r += AYInfo.chip1.out[2] + AYInfo.chip1.out[1] / 2;
	}

	if (out_l > 32767)
		out_l = 32767;
	if (out_r > 32767)
		out_r = 32767;

	return out_l | (out_r << 16);
}

void pt3_init() {
	memset(&AYInfo, 0, sizeof(AYInfo));
	ay_init(&AYInfo.chip0);
	ay_init(&AYInfo.chip1);
	AYInfo.module = (unsigned char*)music_data;
	AYInfo.module_len = music_data_size;
	PT3_Init(&AYInfo);
}

uint32_t pt3_play_16() {
	return emulate_sample();
}

uint32_t pt3_play_16_mono() {
	uint32_t out = pt3_play_16();
	return ((out & 0xffff)) + ((out & 0xffff0000) >> 16); //convert to 16-bit mono
}

uint32_t pt3_play() {
	uint32_t out = pt3_play_16();
	return ((out & 0xff00) >> 8) + ((out & 0xff000000) >> 24); //convert to 8-bit mono
}


#if 0
int main() {
	pt3_init();

	FILE *fp = fopen("out_shiru.wav", "wb");
	uint32_t samples = 44100 * 60;
	for (uint32_t t = 0; t < samples; t++) {
		//uint32_t amp = pt3_play_16();

		uint32_t amp = emulate_sample();

		if (t == 0) {
			int hdr[] = { 1179011410, 36, 1163280727, 544501094, 16, 131073, 44100, 176400, 1048580, 1635017060, 0 };
			hdr[1] = samples * sizeof(amp) + 15;
			hdr[10] = samples * sizeof(amp);
			fwrite(hdr, 1, sizeof(hdr), fp);
		}
		fwrite(&amp, 1, sizeof(amp), fp);
	}
	fclose(fp);
}
#endif