/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"

/* USER CODE BEGIN Includes */

#include <string.h>
#include <stdbool.h>


#include "ssd1306.h"

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
//SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim1_ch1;

UART_HandleTypeDef huart2;


I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

#define SIN_LOOKUP_TABLE_SIZE 360
    
const float sin_lookup_table[SIN_LOOKUP_TABLE_SIZE] = {
    0.0,0.01745240643728351,0.03489949670250097,0.05233595624294383,0.0697564737441253,0.08715574274765817,0.10452846326765346,0.12186934340514748,0.13917310096006544,0.15643446504023087,
    0.17364817766693033,0.1908089953765448,0.20791169081775931,0.224951054343865,0.24192189559966773,0.25881904510252074,0.27563735581699916,0.29237170472273677,0.3090169943749474,0.32556815445715664,
    0.3420201433256687,0.35836794954530027,0.374606593415912,0.3907311284892737,0.40673664307580015,0.42261826174069944,0.4383711467890774,0.45399049973954675,0.4694715627858908,0.48480962024633706,
    0.49999999999999994,0.5150380749100542,0.5299192642332049,0.5446390350150271,0.5591929034707469,0.573576436351046,0.5877852522924731,0.6018150231520483,0.6156614753256582,0.6293203910498374,
    0.6427876096865393,0.6560590289905072,0.6691306063588582,0.6819983600624985,0.6946583704589973,0.7071067811865475,0.7193398003386511,0.7313537016191705,0.7431448254773941,0.754709580222772,
    0.766044443118978,0.7771459614569708,0.788010753606722,0.7986355100472928,0.8090169943749475,0.8191520442889918,0.8290375725550417,0.8386705679454239,0.848048096156426,0.8571673007021122,
    0.8660254037844386,0.8746197071393957,0.8829475928589269,0.8910065241883678,0.898794046299167,0.9063077870366499,0.9135454576426009,0.9205048534524403,0.9271838545667874,0.9335804264972017,
    0.9396926207859083,0.9455185755993167,0.9510565162951535,0.9563047559630354,0.9612616959383189,0.9659258262890683,0.9702957262759965,0.9743700647852352,0.9781476007338056,0.981627183447664,
    0.984807753012208,0.9876883405951378,0.9902680687415703,0.992546151641322,0.9945218953682733,0.9961946980917455,0.9975640502598242,0.9986295347545738,0.9993908270190958,0.9998476951563913,
    1.0,0.9998476951563913,0.9993908270190958,0.9986295347545738,0.9975640502598242,0.9961946980917455,0.9945218953682734,0.9925461516413221,0.9902680687415704,0.9876883405951377,
    0.984807753012208,0.981627183447664,0.9781476007338057,0.9743700647852352,0.9702957262759965,0.9659258262890683,0.9612616959383189,0.9563047559630355,0.9510565162951536,0.9455185755993168,
    0.9396926207859084,0.9335804264972017,0.9271838545667874,0.9205048534524404,0.913545457642601,0.90630778703665,0.8987940462991669,0.8910065241883679,0.8829475928589271,0.8746197071393959,
    0.8660254037844387,0.8571673007021123,0.8480480961564261,0.8386705679454239,0.8290375725550417,0.819152044288992,0.8090169943749475,0.7986355100472927,0.788010753606722,0.777145961456971,
    0.766044443118978,0.7547095802227718,0.7431448254773942,0.7313537016191706,0.7193398003386514,0.7071067811865476,0.6946583704589971,0.6819983600624986,0.6691306063588583,0.6560590289905073,
    0.6427876096865395,0.6293203910498377,0.6156614753256584,0.6018150231520482,0.5877852522924732,0.5735764363510464,0.5591929034707469,0.544639035015027,0.5299192642332049,0.5150380749100544,
    0.49999999999999994,0.48480962024633717,0.4694715627858911,0.45399049973954686,0.4383711467890773,0.4226182617406995,0.40673664307580043,0.39073112848927416,0.37460659341591224,0.3583679495453002,
    0.3420201433256689,0.32556815445715703,0.3090169943749475,0.29237170472273705,0.27563735581699966,0.258819045102521,0.24192189559966773,0.22495105434386478,0.20791169081775931,0.19080899537654497,
    0.17364817766693028,0.15643446504023098,0.13917310096006574,0.12186934340514755,0.10452846326765373,0.08715574274765864,0.06975647374412552,0.05233595624294381,0.0348994967025007,0.01745240643728344,
    1.2246467991473532e-16,-0.017452406437283192,-0.0348994967025009,-0.05233595624294356,-0.06975647374412483,-0.08715574274765794,-0.10452846326765305,-0.12186934340514774,-0.13917310096006552,-0.15643446504023073,
    -0.17364817766693047,-0.19080899537654472,-0.20791169081775907,-0.22495105434386498,-0.2419218955996675,-0.25881904510252035,-0.275637355816999,-0.2923717047227364,-0.30901699437494773,-0.32556815445715676,
    -0.34202014332566866,-0.35836794954530043,-0.374606593415912,-0.39073112848927355,-0.4067366430757998,-0.4226182617406993,-0.43837114678907707,-0.45399049973954625,-0.46947156278589086,-0.48480962024633695,
    -0.5000000000000001,-0.5150380749100542,-0.5299192642332048,-0.5446390350150271,-0.5591929034707467,-0.5735764363510458,-0.587785252292473,-0.601815023152048,-0.6156614753256578,-0.6293203910498376,
    -0.6427876096865393,-0.6560590289905074,-0.6691306063588582,-0.6819983600624984,-0.6946583704589974,-0.7071067811865475,-0.7193398003386509,-0.7313537016191701,-0.743144825477394,-0.7547095802227717,
    -0.7660444431189779,-0.7771459614569711,-0.7880107536067221,-0.7986355100472928,-0.8090169943749473,-0.8191520442889916,-0.8290375725550414,-0.838670567945424,-0.848048096156426,-0.8571673007021121,
    -0.8660254037844384,-0.874619707139396,-0.882947592858927,-0.8910065241883678,-0.8987940462991668,-0.9063077870366497,-0.913545457642601,-0.9205048534524403,-0.9271838545667873,-0.9335804264972016,
    -0.9396926207859082,-0.9455185755993168,-0.9510565162951535,-0.9563047559630353,-0.961261695938319,-0.9659258262890683,-0.9702957262759965,-0.9743700647852351,-0.9781476007338056,-0.9816271834476639,
    -0.984807753012208,-0.9876883405951377,-0.9902680687415704,-0.9925461516413221,-0.9945218953682734,-0.9961946980917455,-0.9975640502598242,-0.9986295347545738,-0.9993908270190957,-0.9998476951563913,
    -1.0,-0.9998476951563913,-0.9993908270190958,-0.9986295347545738,-0.9975640502598243,-0.9961946980917455,-0.9945218953682734,-0.992546151641322,-0.9902680687415704,-0.9876883405951378,
    -0.9848077530122081,-0.9816271834476641,-0.9781476007338058,-0.9743700647852352,-0.9702957262759966,-0.9659258262890682,-0.9612616959383188,-0.9563047559630354,-0.9510565162951536,-0.945518575599317,
    -0.9396926207859085,-0.9335804264972021,-0.9271838545667874,-0.9205048534524405,-0.9135454576426008,-0.9063077870366499,-0.898794046299167,-0.8910065241883679,-0.8829475928589271,-0.8746197071393961,
    -0.8660254037844386,-0.8571673007021123,-0.8480480961564262,-0.8386705679454243,-0.8290375725550421,-0.8191520442889918,-0.8090169943749476,-0.798635510047293,-0.7880107536067218,-0.7771459614569708,
    -0.7660444431189781,-0.7547095802227722,-0.7431448254773946,-0.731353701619171,-0.7193398003386517,-0.7071067811865477,-0.6946583704589976,-0.6819983600624983,-0.6691306063588581,-0.6560590289905074,
    -0.6427876096865396,-0.6293203910498378,-0.6156614753256588,-0.6018150231520483,-0.5877852522924734,-0.5735764363510465,-0.5591929034707473,-0.544639035015027,-0.5299192642332058,-0.5150380749100545,
    -0.5000000000000004,-0.4848096202463369,-0.4694715627858908,-0.45399049973954697,-0.438371146789077,-0.4226182617407,-0.40673664307580015,-0.3907311284892747,-0.37460659341591235,-0.35836794954530077,
    -0.3420201433256686,-0.32556815445715753,-0.3090169943749476,-0.29237170472273627,-0.2756373558169998,-0.2588190451025207,-0.24192189559966787,-0.22495105434386534,-0.20791169081775987,-0.19080899537654467,
    -0.17364817766693127,-0.15643446504023112,-0.13917310096006588,-0.12186934340514811,-0.10452846326765342,-0.08715574274765832,-0.06975647374412476,-0.05233595624294437,-0.034899496702500823,-0.01745240643728445,
};

typedef struct
{
    uint16_t freq_hz;   //between 200 and 20khz
    uint16_t duration_ms;
    uint8_t volume;
} tone_t;

#define PWM_DMA_BUFFER_SIZE 1024
__IO uint16_t pwm_dma_buffer[PWM_DMA_BUFFER_SIZE];

#define PWM_FREQ 31250
#define PWM_PERIOD  1024

//FATFS fatfs;
//FIL MyFile;

#define FATFS_BUFFER_SIZE 512
uint8_t fatfs_buffer[FATFS_BUFFER_SIZE];
uint8_t header[44];

volatile bool pwm_dma_ready, pwm_dma_lower_half, end_of_file;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
//static void MX_SPI2_Init(void);
static void MX_TIM2_Init(void);
//static void MX_USART2_UART_Init(void);


static void MX_I2C1_Init(void);

                                    
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
                                

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if(end_of_file)
    {
        HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
        HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
    }
    pwm_dma_ready = true;
    pwm_dma_lower_half = false;
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}

void HAL_TIM_PWM_PulseHalfFinishedCallback(DMA_HandleTypeDef *hdma)
{
    if(end_of_file)
    {
        HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
        HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
    }
    pwm_dma_ready = true;
    pwm_dma_lower_half = true;
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}

void play_tone(tone_t tone)
{
    uint16_t buffer_size = PWM_FREQ / tone.freq_hz;
    
    for(uint32_t i = 0; i < buffer_size; i++)
    {
        pwm_dma_buffer[i] = sin_lookup_table[i * SIN_LOOKUP_TABLE_SIZE / buffer_size] * tone.volume/2;
    }
    
    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwm_dma_buffer, buffer_size);

    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    
    HAL_Delay(tone.duration_ms);
    
    HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2)
    {
        //disk_timerproc();
    }
}

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
 
/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART */
    //HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 100);
    
    
    return ch;
}

void pwm_dma_fill_buffer(uint8_t *wav_data, uint16_t data_size, bool lower_half)
{
    int32_t pcm_value;
    uint16_t buffer_start = lower_half ? 0 : FATFS_BUFFER_SIZE/2;
    
    for(uint16_t i = 0; i < data_size; i+=2)
    {
        pcm_value = (int32_t)(wav_data[i] + (wav_data[i+1]<<8));
        pwm_dma_buffer[i/2 + buffer_start] = ((uint16_t)(pcm_value + 32768))>>5;
    }
}

void playback(const char* path)
{
#if 0
    if(f_open(&MyFile, path, FA_OPEN_EXISTING | FA_READ) != FR_OK)
    {
        printf("failed to open file\n");
        return;
    }
    printf("File opened\n");
    
    uint32_t bytesread;
    FRESULT res;
    
    //read header (44 bytes)
    res = f_read(&MyFile, header, sizeof(header), (UINT*)&bytesread);
    
    if((bytesread < sizeof(header)) || (res != FR_OK))
    {
        printf("Failed to read file\n");
        return;
    }

    uint16_t channels = header[22] + (header[23]<<8);
    uint32_t sample_rate = header[24] + (header[25]<<8) + (header[26]<<16) + (header[27]<<24);
    uint16_t bits_per_sample = header[34] + (header[35]<<8);
    
    printf("channels: %d\n", channels);
    printf("sample rate: %d\n", sample_rate);
    printf("bits per sample: %d\n", bits_per_sample);
    
    if(channels != 1 || sample_rate != 32000 || bits_per_sample != 16)
    {
        printf("Type of wav encoding is not supported in current version.\n");
        return;
    }
    
    uint32_t count = 0;
    
    //fill up pwm buffer before we start
    
    //lower half
    res = f_read(&MyFile, fatfs_buffer, FATFS_BUFFER_SIZE, (UINT*)&bytesread);
    if(res != FR_OK)
    {
        printf("Read failed, count: %d\n", count);
        end_of_file = true;
    }
    
    count++;
    pwm_dma_fill_buffer(fatfs_buffer, bytesread, true);
    
    //upper half
    res = f_read(&MyFile, fatfs_buffer, FATFS_BUFFER_SIZE, (UINT*)&bytesread);
    if(res != FR_OK)
    {
        printf("Read failed, count: %d\n", count);
        end_of_file = true;
    }
    
    count++;
    pwm_dma_fill_buffer(fatfs_buffer, bytesread, false);
    
    //start PWM, CH1 and CH1N
    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwm_dma_buffer, FATFS_BUFFER_SIZE);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    
    pwm_dma_ready = false;
    
    //loop until the end of file
    while(end_of_file == false)
    {
        if(pwm_dma_ready)
        {
            res = f_read(&MyFile, fatfs_buffer, FATFS_BUFFER_SIZE, (UINT*)&bytesread);
            if(res != FR_OK)
            {
                printf("Read failed, count: %d\n", count);
                HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
                HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
                return;
            }
            
            if(f_eof(&MyFile))
            {
                printf("end of file\n");
                end_of_file = true;
            }
            
            pwm_dma_fill_buffer(fatfs_buffer, bytesread, pwm_dma_lower_half);
            
            pwm_dma_ready = false;
            count++;
        }
    }
    printf("playback done\n");
#endif
}

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

int wish_melody[] = {
  NOTE_B3, 
  NOTE_F4, NOTE_F4, NOTE_G4, NOTE_F4, NOTE_E4,
  NOTE_D4, NOTE_D4, NOTE_D4,
  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_F4,
  NOTE_E4, NOTE_E4, NOTE_E4,
  NOTE_A4, NOTE_A4, NOTE_B4, NOTE_A4, NOTE_G4,
  NOTE_F4, NOTE_D4, NOTE_B3, NOTE_B3,
  NOTE_D4, NOTE_G4, NOTE_E4,
  NOTE_F4
};

int wish_tempo[] = {
  4,
  4, 8, 8, 8, 8,
  4, 4, 4,
  4, 8, 8, 8, 8,
  4, 4, 4,
  4, 8, 8, 8, 8,
  4, 4, 8, 8,
  4, 4, 4,
  2
};

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  //MX_FATFS_Init();
//  MX_SPI2_Init();
  MX_TIM2_Init();
//  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
    
//    printf("Up and running\r\n");
    
    htim1.hdma[TIM_DMA_ID_CC1]->XferHalfCpltCallback = HAL_TIM_PWM_PulseHalfFinishedCallback;
    HAL_TIM_Base_Start_IT(&htim2);

#if 0
    if (f_mount(&fatfs, USER_Path, 0) != FR_OK)
    {
        printf("error in mounting disk\n");
    }
    else
    {
        printf("disk mounted\n");
        playback("test.wav");
    }
#endif
    

    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_SetCursor(9,19);
    ssd1306_WriteString("Merry Xmas!", Font_7x10, White);
    ssd1306_UpdateScreen();


	int notes = sizeof(wish_melody)/sizeof(wish_melody[0]);

	for (int i = 0; i<notes; i++) {
		tone_t t;
		t.freq_hz = wish_melody[i];
		t.duration_ms = 1000/wish_tempo[i];
		t.volume = 1;
		play_tone(t);
	}

    //play_tone((tone_t){880, 100, 4});
    //play_tone((tone_t){1000, 100, 4});


    
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

		//HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		//HAL_Delay(500);

//    play_tone((tone_t){880, 100, 4});
//    play_tone((tone_t){1000, 100, 4});

	HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
	HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  }
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* SPI2 init function */
/*
static void MX_SPI2_Init(void)
{
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;

  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
}
*/

/* TIM1 init function */
static void MX_TIM1_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_OC_InitTypeDef sConfigOC;
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 2047;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim1);

}

/* TIM2 init function */
static void MX_TIM2_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 64000;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 10;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

}

/* USART2 init function */
/*
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}
*/

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);


  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}


static void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000*4;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

}


/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
