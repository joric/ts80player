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

#define CROP_OSC 0
#define RENDER_TO_DMA 1

#include <stdio.h> // snprintf
volatile int errorcode = 0;

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim3_ch1;

I2C_HandleTypeDef hi2c1;

#define PWM_DMA_BUFFER_SIZE 1024
__IO uint16_t pwm_dma_buffer[PWM_DMA_BUFFER_SIZE];

uint32_t g_buffer_size = PWM_DMA_BUFFER_SIZE;

#define PWM_FREQ 31250
#define PWM_PERIOD  1024

#define FATFS_BUFFER_SIZE 512
uint8_t fatfs_buffer[FATFS_BUFFER_SIZE];

volatile bool pwm_dma_ready, pwm_dma_lower_half, end_of_file;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	//errorcode = 667; //called ok
    if(end_of_file)
    {
        HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);
        HAL_TIMEx_PWMN_Stop(&htim3, TIM_CHANNEL_1);
    }
    pwm_dma_ready = true;
    pwm_dma_lower_half = false;
}

void HAL_TIM_PWM_PulseHalfFinishedCallback(DMA_HandleTypeDef *hdma)
{
	errorcode = 666; // never called ? why ? should investigate

    if(end_of_file)
    {
        HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);
        HAL_TIMEx_PWMN_Stop(&htim3, TIM_CHANNEL_1);
    }
    pwm_dma_ready = true;
    pwm_dma_lower_half = true;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2)
    {
        //disk_timerproc();
    }
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


/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

void drawOsc(void) {
	ssd1306_Fill(Black);

	const int w = 96, h = 16;

#if CROP_OSC
	uint16_t window_size = FATFS_BUFFER_SIZE/2;
	uint16_t buffer_start = pwm_dma_lower_half ? 0 : FATFS_BUFFER_SIZE/2;
#else
	uint16_t window_size = g_buffer_size;
	uint16_t buffer_start = 0;
#endif

	uint16_t maxamp = 0;
	for (int x = 0; x<w; x++) {
		uint16_t amp = pwm_dma_buffer[buffer_start + x * window_size / w];
		if (amp>maxamp) maxamp = amp;
	}

	for (int x = 0; x<w; x++) {
		uint16_t amp = pwm_dma_buffer[buffer_start + x * window_size / w];
		int y = amp * h / maxamp;
		ssd1306_DrawPixel(x, y+16, White);
	}

	char buff[64];
	ssd1306_SetCursor(0, 0);
	snprintf(buff, sizeof(buff), "Playing: %d", errorcode);
	ssd1306_WriteString(buff, Font_7x10, White);

	ssd1306_UpdateScreen();
}

// see my oneliner music collection https://pastebin.com/uDvJgZ1a
// test it here: http://wurstcaptures.untergrund.net/music/
//#define MUSIC(t) t // basic saw
//#define MUSIC(t) ((t%128)<64?0:0xff) // square
//#define MUSIC(t) (t%31337>>3)|(t|t>>7)
//#define MUSIC(t) t*(((t>>12)|(t>>8))&(63&(t>>4)))
#define MUSIC(t) ((-t&4095)*(255&t*(t&(t>>13)))>>12)+(127&t*(234&t>>8&t>>3)>>(3&t>>14))

int vf_read1(uint8_t * buffer, uint32_t buffer_size, uint32_t * bytesread) {
	static uint32_t t;
	for (int i=0; i<buffer_size/2; i++) {
		int32_t amp = MUSIC(t);
		amp = (amp * 0xff) * 128;
		buffer[i*2+0] = amp & 0xff;
		buffer[i*2+1] = (amp >> 8) & 0xff;
		t++;
	}
	* bytesread = buffer_size;
	return 0;
}


//#include "fatal.pt3.h"
//#define pt3_data fatal_pt3
//#define pt3_size fatal_pt3_len

#include "rage.pt3.h"
#define pt3_data rage_pt3
#define pt3_size rage_pt3_len

extern uint32_t pt3_player(uint32_t sample, uint32_t rate, uint8_t * data, uint32_t size, uint32_t * samples);

int vf_read2(uint8_t * buffer, uint32_t buffer_size, uint32_t * bytesread) {
	uint32_t samples;
	static uint32_t t;
	for (int i=0; i<buffer_size/2; i++) {
		int32_t amp = pt3_player(t, PWM_FREQ, pt3_data, pt3_size, &samples);
		uint16_t mix_l = amp & 0xffff;
		uint16_t mix_r = (amp >> 16);
		amp = (mix_l/2 + mix_r/2);
		buffer[i*2+0] = amp & 0xff;
		buffer[i*2+1] = (amp >> 8) & 0xff;
		t++;
	}
	* bytesread = buffer_size;
	return 0;
}

#define vf_read vf_read2

#if 1

void playback(void) {
	uint32_t bytesread = 0;
	uint32_t count = 0;

	//lower half
	vf_read(fatfs_buffer, FATFS_BUFFER_SIZE, &bytesread);
	count++;
	pwm_dma_fill_buffer(fatfs_buffer, bytesread, true);

	//upper half
	vf_read(fatfs_buffer, FATFS_BUFFER_SIZE, &bytesread);
	count++;
	pwm_dma_fill_buffer(fatfs_buffer, bytesread, true);

	//start PWM, CH3 and CH3N
	HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t *)pwm_dma_buffer, FATFS_BUFFER_SIZE);
	HAL_TIMEx_PWMN_Start(&htim3, TIM_CHANNEL_1);
	pwm_dma_ready = false;


	for (;;) {
		if(pwm_dma_ready) {
#if RENDER_TO_DMA
			uint32_t samples;
			static uint32_t t;
			for (int i=0; i<PWM_DMA_BUFFER_SIZE; i++) {

				int32_t amp = pt3_player(t, PWM_FREQ/2, pt3_data, pt3_size, &samples);

				uint16_t mix_l = amp & 0xffff;
				uint16_t mix_r = (amp >> 16);

				pwm_dma_buffer[i] = (mix_l + mix_r) / 512;
				t++;
			}
#else
			vf_read(fatfs_buffer, FATFS_BUFFER_SIZE, &bytesread);
			pwm_dma_fill_buffer(fatfs_buffer, bytesread, pwm_dma_lower_half);
#endif
			drawOsc();
			pwm_dma_ready = false;
			HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

		}
	}
}
#endif

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
  MX_TIM3_Init();
  MX_TIM2_Init();

  /* USER CODE BEGIN 2 */

  htim3.hdma[TIM_DMA_ID_CC1]->XferHalfCpltCallback = HAL_TIM_PWM_PulseHalfFinishedCallback;
  HAL_TIM_Base_Start_IT(&htim2);

  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(9,19);
  ssd1306_WriteString("Playing...", Font_7x10, White);
  ssd1306_UpdateScreen();

  playback();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

	// blink
	HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	HAL_Delay(1000);

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


/* TIM3 init function */
static void MX_TIM3_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_OC_InitTypeDef sConfigOC;
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 2047;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.RepetitionCounter = 0;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
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
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
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
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim3, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim3);

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

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

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

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
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

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
