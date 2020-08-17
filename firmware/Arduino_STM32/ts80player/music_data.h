#ifndef PROGMEM
#define PROGMEM
#endif

#if APPLE_PT3
#include "nq_bad_apple.pt3.h" // 6-channel
#define music_data      nq_bad_apple_pt3
#define music_data_size nq_bad_apple_pt3_len

#elif FATAL_PT3
#include "fatal.pt3.h"
#define music_data      fatal_pt3
#define music_data_size fatal_pt3_len

#else
#include "rage.pt3.h"
#define music_data      rage_pt3
#define music_data_size rage_pt3_len
#endif
