#include "main.h"

extern DMA_HandleTypeDef hdma_adc1;
extern DMA_HandleTypeDef hdma_adc2;
extern TIM_HandleTypeDef htim2;

static uint32_t HAL_RCC_ADC12_CLK_ENABLED = 0;

void HAL_MspInit(void)
{
__HAL_RCC_SYSCFG_CLK_ENABLE();
__HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	if (hadc->Instance == ADC1) {
		HAL_RCC_ADC12_CLK_ENABLED++;

		if (HAL_RCC_ADC12_CLK_ENABLED == 1)
			__HAL_RCC_ADC12_CLK_ENABLE();

		__HAL_RCC_GPIOC_CLK_ENABLE();

		GPIO_InitStruct.Pin = GPIO_PIN_0;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		hdma_adc1.Instance = DMA1_Channel1;
		hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
		hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
		hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
		hdma_adc1.Init.Mode = DMA_CIRCULAR;
		hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
		if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
			error_handler();

		__HAL_LINKDMA(hadc, DMA_Handle, hdma_adc1);
	}
	else if (hadc->Instance == ADC2) {
		HAL_RCC_ADC12_CLK_ENABLED++;

		if (HAL_RCC_ADC12_CLK_ENABLED == 1)
			__HAL_RCC_ADC12_CLK_ENABLE();

		__HAL_RCC_GPIOC_CLK_ENABLE();

		GPIO_InitStruct.Pin = GPIO_PIN_1;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		hdma_adc2.Instance = DMA2_Channel1;
		hdma_adc2.Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_adc2.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_adc2.Init.MemInc = DMA_MINC_ENABLE;
		hdma_adc2.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
		hdma_adc2.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
		hdma_adc2.Init.Mode = DMA_CIRCULAR;
		hdma_adc2.Init.Priority = DMA_PRIORITY_LOW;
		if (HAL_DMA_Init(&hdma_adc2) != HAL_OK)
			error_handler();

		__HAL_LINKDMA(hadc, DMA_Handle, hdma_adc2);
	}
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
	if (hadc->Instance == ADC1) {
		HAL_RCC_ADC12_CLK_ENABLED--;

		if (HAL_RCC_ADC12_CLK_ENABLED == 0)
			__HAL_RCC_ADC12_CLK_DISABLE();

		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0);

		HAL_DMA_DeInit(hadc->DMA_Handle);
	}
	else if (hadc->Instance == ADC2) {
		HAL_RCC_ADC12_CLK_ENABLED--;

		if (HAL_RCC_ADC12_CLK_ENABLED == 0)
			__HAL_RCC_ADC12_CLK_DISABLE();

		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1);

		HAL_DMA_DeInit(hadc->DMA_Handle);
	}
}

void HAL_DAC_MspInit(DAC_HandleTypeDef* hdac)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if(hdac->Instance == DAC)
	{
		__HAL_RCC_DAC1_CLK_ENABLE();

		__HAL_RCC_GPIOA_CLK_ENABLE();

		GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	}
}

void HAL_DAC_MspDeInit(DAC_HandleTypeDef* hdac)
{
	if(hdac->Instance == DAC)
	{
		__HAL_RCC_DAC1_CLK_DISABLE();

		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4);
	}
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
	if(htim_base->Instance == TIM2)
	{
		__HAL_RCC_TIM2_CLK_ENABLE();
		HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(TIM2_IRQn);
	}
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
	if(htim_base->Instance == TIM2)
	{
		__HAL_RCC_TIM2_CLK_DISABLE();
		HAL_NVIC_DisableIRQ(TIM2_IRQn);
	}

}
