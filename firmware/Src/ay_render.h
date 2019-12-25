// minimal libayemu-based AY sound renderer
// data converted with psg_to_text.py from ayumi

#include "frame_data.h"

#define AYEMU_MAX_AMP 24575
#define AYEMU_DEFAULT_CHIP_FREQ 1773400

#include <stdio.h>
#include <malloc.h>
typedef unsigned char ayemu_ay_reg_frame_t[14];

typedef struct {
	int tone_a;		/**< R0, R1 */
	int tone_b;		/**< R2, R3 */
	int tone_c;		/**< R4, R5 */
	int noise;		/**< R6 */
	int R7_tone_a;	/**< R7 bit 0 */
	int R7_tone_b;	/**< R7 bit 1 */
	int R7_tone_c;	/**< R7 bit 2 */
	int R7_noise_a;	/**< R7 bit 3 */
	int R7_noise_b;	/**< R7 bit 4 */
	int R7_noise_c;	/**< R7 bit 5 */
	int vol_a;		/**< R8 bits 3-0 */
	int vol_b;		/**< R9 bits 3-0 */
	int vol_c;		/**< R10 bits 3-0 */
	int env_a;		/**< R8 bit 4 */
	int env_b;		/**< R9 bit 4 */
	int env_c;		/**< R10 bit 4 */
	int env_freq;	/**< R11, R12 */
	int env_style;	/**< R13 */
} ayemu_regdata_t;


typedef struct {
	int freq;			/**< sound freq */
	int channels;		/**< channels (1-mono, 2-stereo) */
	int bpc;			/**< bits (8 or 16) */
} ayemu_sndfmt_t;

typedef struct {
	int table[32];			/**< table of volumes for chip */
	int ChipFreq;			/**< chip emulator frequency */
	int eq[6];				/**< volumes for channels, range -100..100 **/
	int bit_a;				/**< state of channel A generator */
	int bit_b;				/**< state of channel B generator */
	int bit_c;				/**< state of channel C generator */
	int bit_n;				/**< current generator state */
	int cnt_a;				/**< back counter of A */
	int cnt_b;				/**< back counter of B */
	int cnt_c;				/**< back counter of C */
	int cnt_n;				/**< back counter of noise generator */
	int cnt_e;				/**< back counter of envelop generator */
	int EnvNum;				/**< number of current envilopment (0...15) */
	int env_pos;			/**< current position in envelop (0...127) */
	int Cur_Seed;			/**< random numbers counter */
	ayemu_regdata_t regs;	/**< parsed registers data */
	ayemu_sndfmt_t sndfmt;	/**< output sound format */
	int ChipTacts_per_outcount;	 /**< chip's counts per one sound signal count */
	int Amp_Global;			/**< scale factor for amplitude */
	int vols[6][32];		/**< stereo type (channel volumes) and chip table. This cache calculated by #table and #eq	*/
} ayemu_ay_t;

#if 0
static int AY_table [16] = {0,513,828,1239,1923,3238,4926,9110,10344,17876,24682,30442,38844,47270,56402,65535};
static const int AY_eq[] = {100,33,70,70,33,100};
#else //YM
static int AY_table [32] = { 0, 0, 190, 286, 375, 470, 560, 664, 866, 1130, 1515, 1803, 2253, 2848, 3351, 3862, 4844, 6058, 7290, 8559, 10474, 12878, 15297, 17787, 21500, 26172, 30866, 35676, 42664, 50986, 58842, 65535};
static const int AY_eq[] = {100, 5, 70, 70, 5, 100};
#endif

#define ENVVOL envelope(ay->regs.env_style, ay->env_pos)


int envelope(int e, int x) {
	int loop = e > 7 && (e % 2)==0;
	int q = (x / 32) & (loop ? 1 : 3);
	int ofs = (q==0 ? (e & 4)==0 : (e == 8 || e==11 || e==13 || e==14)) ? 31 : 0;
	return (q==0 || loop) ? ( ofs + (ofs!=0 ? -1 : 1) * (x % 32) ) : ofs;
}


void ayemu_init(ayemu_ay_t *ay, int freq, int channels, int bits) {
	ay->ChipFreq = AYEMU_DEFAULT_CHIP_FREQ;

	ay->cnt_a = ay->cnt_b = ay->cnt_c = ay->cnt_n = ay->cnt_e = 0;
	ay->bit_a = ay->bit_b = ay->bit_c = ay->bit_n = 0;
	ay->env_pos = ay->EnvNum = 0;
	ay->Cur_Seed = 0xffff;

	for (int n = 0; n < 32; n++)
		ay->table[n] = AY_table[ n * (sizeof(AY_table) / sizeof(AY_table[0])) / 32 ];

	for (int i = 0 ; i < 6 ; i++)
		ay->eq[i] = AY_eq[i];

	ay->sndfmt.freq = freq;
	ay->sndfmt.channels = channels;
	ay->sndfmt.bpc = bits;
}


void ayemu_set_regs(ayemu_ay_t *ay, ayemu_ay_reg_frame_t regs) {
	ay->regs.tone_a = regs[0] + ((regs[1]&0x0f) << 8);
	ay->regs.tone_b = regs[2] + ((regs[3]&0x0f) << 8);
	ay->regs.tone_c = regs[4] + ((regs[5]&0x0f) << 8);
	ay->regs.noise = regs[6] & 0x1f;
	ay->regs.R7_tone_a = ! (regs[7] & 0x01);
	ay->regs.R7_tone_b = ! (regs[7] & 0x02);
	ay->regs.R7_tone_c = ! (regs[7] & 0x04);
	ay->regs.R7_noise_a = ! (regs[7] & 0x08);
	ay->regs.R7_noise_b = ! (regs[7] & 0x10);
	ay->regs.R7_noise_c = ! (regs[7] & 0x20);
	ay->regs.vol_a = regs[8] & 0x0f;
	ay->regs.vol_b = regs[9] & 0x0f;
	ay->regs.vol_c = regs[10] & 0x0f;
	ay->regs.env_a = regs[8] & 0x10;
	ay->regs.env_b = regs[9] & 0x10;
	ay->regs.env_c = regs[10] & 0x10;
	ay->regs.env_freq = regs[11] + (regs[12] << 8);
	if (regs[13] != 0xff) {
		ay->regs.env_style = regs[13] & 0x0f;
		ay->env_pos = ay->cnt_e = 0;
	}
}

void ayemu_gen_sound(ayemu_ay_t *ay, volatile uint16_t * frame_buf, uint32_t samples)
{
	int mix_l, mix_r;
	int tmpvol;
	int m;

	int vol, max_l, max_r;

	ay->ChipTacts_per_outcount = ay->ChipFreq / ay->sndfmt.freq / 8;

	for (int n = 0; n < 32; n++) {
		vol = ay->table[n];
		for (int m=0; m < 6; m++)
			ay->vols[m][n] = (int) (((double) vol * ay->eq[m]) / 100);
	}

	max_l = ay->vols[0][31] + ay->vols[2][31] + ay->vols[3][31];
	max_r = ay->vols[1][31] + ay->vols[3][31] + ay->vols[5][31];

	vol = (max_l > max_r) ? max_l : max_r;
	ay->Amp_Global = ay->ChipTacts_per_outcount *vol / AYEMU_MAX_AMP;

	while (samples-- > 0) {

		mix_l = mix_r = 0;

		for (m = 0 ; m < ay->ChipTacts_per_outcount ; m++) {

			if (++ay->cnt_a >= ay->regs.tone_a) {
				ay->cnt_a = 0;
				ay->bit_a = ! ay->bit_a;
			}

			if (++ay->cnt_b >= ay->regs.tone_b) {
				ay->cnt_b = 0;
				ay->bit_b = ! ay->bit_b;
			}

			if (++ay->cnt_c >= ay->regs.tone_c) {
				ay->cnt_c = 0;
				ay->bit_c = ! ay->bit_c;
			}

			if (++ay->cnt_n >= (ay->regs.noise * 2)) {
				ay->cnt_n = 0;
				ay->Cur_Seed = (ay->Cur_Seed * 2 + 1) ^ (((ay->Cur_Seed >> 16) ^ (ay->Cur_Seed >> 13)) & 1);
				ay->bit_n = ((ay->Cur_Seed >> 16) & 1);
			}
			
			if (++ay->cnt_e >= ay->regs.env_freq) {
				ay->cnt_e = 0;
				if (++ay->env_pos > 127) ay->env_pos = 64;
			}

			if ((ay->bit_a | !ay->regs.R7_tone_a) & (ay->bit_n | !ay->regs.R7_noise_a)) {
				tmpvol = (ay->regs.env_a)? ENVVOL : ay->regs.vol_a * 2 + 1;
				mix_l += ay->vols[0][tmpvol];
				mix_r += ay->vols[1][tmpvol];
			}
			
			if ((ay->bit_b | !ay->regs.R7_tone_b) & (ay->bit_n | !ay->regs.R7_noise_b)) {
				tmpvol =(ay->regs.env_b)? ENVVOL :	ay->regs.vol_b * 2 + 1;
				mix_l += ay->vols[2][tmpvol];
				mix_r += ay->vols[3][tmpvol];
			}
			
			if ((ay->bit_c | !ay->regs.R7_tone_c) & (ay->bit_n | !ay->regs.R7_noise_c)) {
				tmpvol = (ay->regs.env_c)? ENVVOL : ay->regs.vol_c * 2 + 1;
				mix_l += ay->vols[4][tmpvol];
				mix_r += ay->vols[5][tmpvol];
			}
		}

		mix_l /= ay->Amp_Global;
		mix_r /= ay->Amp_Global;

		int32_t pcm_value = (mix_l/2 + mix_r/2) - 32768;


		*frame_buf++ = ((uint16_t)(pcm_value + 32768))>>5      >>3; //too loud
	}
}

/*
int main() {
	ayemu_ay_t ay;
	ayemu_ay_reg_frame_t regs;

	int freq = 44100;
	int channels = 2;
	int bits = 16;
	int playerFreq = 50;
	int frames = sizeof(frame_data)/sizeof(frame_data[0]);

	ayemu_init(&ay, freq, channels, bits);
	int audio_bufsize = freq * channels * (bits / 8) / playerFreq;
	unsigned char * audio_buf = (unsigned char*)malloc(audio_bufsize);

	printf("frames: %d, writing wav...\n", frames);

	FILE *fp = fopen("out.wav", "wb");
	int hdr[] = {1179011410, 36, 1163280727, 544501094, 16, 131073, 44100, 176400, 1048580, 1635017060, 0};
	hdr[1] = audio_bufsize * frames+15;
	hdr[10] = audio_bufsize * frames;
	fwrite(hdr, 1, sizeof(hdr), fp);

	size_t pos = 0;
	while (pos++ < frames) {
		ayemu_set_regs (&ay, frame_data[pos-1]);
		ayemu_gen_sound (&ay, audio_buf, audio_bufsize);
		fwrite(audio_buf, 1, audio_bufsize, fp);
	}

	fclose(fp);
}
*/
