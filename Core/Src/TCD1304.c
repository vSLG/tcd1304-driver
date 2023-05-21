/*
 * TCD1304.c
 *
 *  Created on: May 21, 2023
 *      Author: slg
 */

#include <memory.h>

#include "TCD1304.h"
#include "stm32f4xx_hal.h"

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_rx;

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;

extern void init_icg_sh();
extern void init_adc_tim();

#define CCDBuffer 3694
volatile uint16_t CCDPixelBuffer[CCDBuffer];

#define SPIRxBufferSize CCDBuffer * 2
uint8_t SPIRxBuffer[SPIRxBufferSize] = { 0 };

int curr_reading = 0;
int flushed = 0;
CCDConfig_t *update_config = NULL;
CCDConfig_t ccd_config;

void tcd1304_flush();
void tcd1304_config_icg_sh();
void tcd1304_stop_timers();

void tcd1304_set_config(CCDConfig_t *cfg) {
	tcd1304_flush();

	memcpy(&ccd_config, cfg, sizeof(CCDConfig_t));

	HAL_TIM_Base_Stop(&htim2);
	HAL_TIM_Base_Stop(&htim5);

	tcd1304_config_icg_sh();
}

void tcd1304_setup() {
	ccd_config.sh_period = 1680;
	ccd_config.icg_period = 630000;

	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); //PA6 - fM

	HAL_SPI_Receive_DMA(&hspi1, SPIRxBuffer, SPIRxBufferSize);
}

void tcd1304_loop() {
	if (!update_config)
		return;

	tcd1304_set_config(update_config);
	update_config = 0;
}

void tcd1304_flush() {
	ccd_config.icg_period = 63000 - 1;
	ccd_config.sh_period = 1680 - 1;

	tcd1304_stop_timers();

	curr_reading = flushed = 0;

	tcd1304_config_icg_sh();

	while (!flushed) {
	}
}

void tcd1304_config_icg_sh() {
	init_icg_sh();
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); //PA0 - ICG
	__HAL_TIM_SET_COUNTER(&htim2, 66); //600 ns delay
	HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3); //PA2 - SH
	HAL_TIM_Base_Start_IT(&htim2);
}

void tcd1304_stop_timers() {
	HAL_ADC_Stop_DMA(&hadc1);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4); //ADC
	HAL_TIM_Base_Stop(&htim2);
	HAL_TIM_Base_Stop(&htim4);
	HAL_TIM_Base_Stop(&htim5);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim != &htim2)
		return;

	if (curr_reading == 3)
		flushed = 1;

	if (curr_reading == 6) {
		init_adc_tim();
		HAL_ADC_Start_DMA(&hadc1, (uint32_t*) CCDPixelBuffer, CCDBuffer);
		HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4); //ADC
	}

	curr_reading++;
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
	HAL_SPI_Receive_DMA(hspi, SPIRxBuffer, SPIRxBufferSize);

	uint8_t *buf = SPIRxBuffer;

	while (!*(uint16_t*) (buf++) == 0x1304)
		;

	update_config = (CCDConfig_t*) (SPIRxBuffer + 2);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	// __asm__("nop");
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	// CDC_Transmit_FS((uint8_t*) CCDPixelBuffer, CCDBuffer);

//	if (!received)
//		return;
//
	HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*) CCDPixelBuffer, CCDBuffer * 2);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}