#include "main.h"
#include "display.h"

#include <math.h>
#include <stdio.h>

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

void systemclock_config(void);
static void dma_init(void);
static void adc1_init(void);
static void gpio_init(void);

uint32_t getadcv(ADC_HandleTypeDef *hadc)
{
	HAL_ADC_Start(hadc);
	HAL_ADC_PollForConversion(hadc, 1);
	return HAL_ADC_GetValue(hadc);
}

// convert index in thermistor specs array to actual temperature
float idxtotemp(size_t idx)
{
	return idx * 5.0 - 55.0;
}

float voltagetotemp(float v)
{
	// 10k ohm thermistor specs. Step is 5 degrees celsius.
	// this values are relation of actual resistance to basic
	// value of the thermistor (which is 10k)
	float t[] = {96.300, 67.010, 47.170, 33.650, 24.260, 17.700,
		13.040, 9.707, 7.293, 5.533, 4.232, 3.265, 2.539, 1.990,
		1.571, 1.249, 1.000, 0.8057, 0.6531, 0.5327, 0.4369,
		0.3603, 0.2986, 0.2488, 0.2083, 0.1752, 0.1481, 0.1258,
		0.1072, 0.09177, 0.07885, 0.068, 0.05112, 0.04454,
		0.03893, 0.03417, 0.03009, 0.02654, 0.02348, 0.02083,
		0.01853, 0.01653};
	size_t cnt;
	size_t b, e, c;
	float r;

	// divide by 0 protection
	if (v < 0.0001)
		return 0.0;

	// calculate thermistor resistance at the moment
	// from voltage value taken from the voltage divideir
	//
	// 5.0 is rr / rt, where rr is the resistors value and
	// rt is the thermistors basic value. In this project
	// 10k thermistor in series with 50k resistor is used:
	//
	// +|---| t |---|50k|---|-
	//            |
	//            | out
	//            |
	r = 5.0 * (1 - v) / v;

	// 
	cnt = sizeof(t) / sizeof(float);

	b = 0;
	e = cnt - 1;

	while (e - b > 1) {
		c = b + (e - b) / 2;

		if (r > t[c])	e = c;
		else		b = c;
	}

	// linear interpolation between two
	// adjacent values in thermistor specs
	return (idxtotemp(e) - 5.0 / (t[b] - t[e]) * (r - t[e]));
}

int display_init()
{
	struct displaypindef pindefs[12];
	
	pindefs[0].gpio = GPIOB;	pindefs[0].pin = GPIO_PIN_0;
	pindefs[1].gpio = GPIOB;	pindefs[1].pin = GPIO_PIN_1;
	pindefs[2].gpio = GPIOB;	pindefs[2].pin = GPIO_PIN_2;
	pindefs[3].gpio = GPIOB;	pindefs[3].pin = GPIO_PIN_3;
	pindefs[4].gpio = GPIOA;	pindefs[4].pin = GPIO_PIN_0;
	pindefs[5].gpio = GPIOA;	pindefs[5].pin = GPIO_PIN_1;
	pindefs[6].gpio = GPIOA;	pindefs[6].pin = GPIO_PIN_2;
	pindefs[7].gpio = GPIOA;	pindefs[7].pin = GPIO_PIN_3;
	pindefs[8].gpio = GPIOA;	pindefs[8].pin = GPIO_PIN_4;
	pindefs[9].gpio = GPIOA;	pindefs[9].pin = GPIO_PIN_5;
	pindefs[10].gpio = GPIOA;	pindefs[10].pin = GPIO_PIN_6;
	pindefs[11].gpio = GPIOA;	pindefs[11].pin = GPIO_PIN_7;

	displayinit(&pindefs);
}

int main(void)
{	
	HAL_Init();

	systemclock_config();

	gpio_init();
	dma_init();
	adc1_init();
	display_init();

	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);


	displayset(DISPLAY_8BIT, DISPLAY_2LINE, DISPLAY_SMALLFONT);
	displayonoff(DISPLAY_ON, DISPLAY_NOCURSOR, DISPLAY_NOBLINK);
	displayclear();
	displayentry(DISPLAY_FORWARD, DISPLAY_NOSHIFTSCREEN);

	while (1) {
		char a[256];
		float v;

		// take voltage from a pin with the thermistor
		// based divider attached
		v = getadcv(&hadc1) / (float) 0x00000fff;

		displaypos(0, 0);

		snprintf(a, 10, "%f", voltagetotemp(v));
		displaystring(a);
		
		HAL_Delay(250);
	}
}

void systemclock_config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		error_handler();

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK
		| RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1
		| RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
			FLASH_LATENCY_2) != HAL_OK)
		error_handler();

	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC12;
	PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV1;
	
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
		error_handler();
}

static void gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	HAL_GPIO_WritePin(GPIOA,
		GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2
		| GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5
		| GPIO_PIN_6 | GPIO_PIN_7,
		GPIO_PIN_RESET);

	HAL_GPIO_WritePin(GPIOB,
		GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
		GPIO_PIN_RESET);

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2
		| GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5
		| GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1
		| GPIO_PIN_2 | GPIO_PIN_3;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

static void dma_init(void)
{
	__HAL_RCC_DMA1_CLK_ENABLE();

	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

static void adc1_init(void)
{
	ADC_MultiModeTypeDef multimode = {0};
	ADC_ChannelConfTypeDef sConfig = {0};

	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.ContinuousConvMode = ENABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DMAContinuousRequests = ENABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;

	if (HAL_ADC_Init(&hadc1) != HAL_OK)
		error_handler();

	multimode.Mode = ADC_MODE_INDEPENDENT;
	if (HAL_ADCEx_MultiModeConfigChannel(&hadc1,
			&multimode) != HAL_OK)
		error_handler();

	sConfig.Channel = ADC_CHANNEL_6;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.SamplingTime = ADC_SAMPLETIME_61CYCLES_5;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
		error_handler();
}

void error_handler(void)
{
	__disable_irq();
	while (1) { }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
