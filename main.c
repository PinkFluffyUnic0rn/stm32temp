#include "main.h"
#include "display.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc2;

TIM_HandleTypeDef htim2;

void systemclock_config(void);
static void dma_init(void);
static void adc1_init(void);
static void adc2_init(void);
static void gpio_init(void);
static void tim2_init(void);

#define TIMPERIOD 0x40
#define TICKSPERSEC (72000000 / 72 / TIMPERIOD)
#define DRAWINTERVAL 1.0

// median filter settings
#define BUFSIZE 4096
#define WINSIZE 11

// flag is set when it is time to draw on screen
int tickdraw = 0;

// samples buffer
float buf[BUFSIZE];
int bufpos = 0;

// pulse count at moment
size_t pulsecnt = 0;

// PWM state
int state = 0;

// singal frequency
size_t pulseslastsec = 0;

uint32_t getadcv(ADC_HandleTypeDef *hadc)
{
	uint32_t v;

	HAL_ADC_Start(hadc);
	HAL_ADC_PollForConversion(hadc, 1);

	v =  HAL_ADC_GetValue(hadc);

	HAL_ADC_Stop(hadc);
	
	return v;
}

// convert index in thermistor specs array to actual temperature
float idxtotemp(size_t idx)
{
	return idx * 5.0 - 55.0;
}

// convert thermistor resistance to temp.
// input resistance is divided by basic value of the thermistor (10000k)
float res2temp(float r)
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

	// binary search through thermistor specs
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

float voltagetotemp(float v)
{
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

	return res2temp(r);
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

float medianfilter(float *buf, size_t winsize)
{
	int lowcnt, highcnt;
	int i;

	lowcnt = highcnt = 0;
	for (i = 0; i < WINSIZE; ++i) {
		if (buf[i] < 0.5f)	++lowcnt;
		else			++highcnt;
	}

	return ((lowcnt >= highcnt) ? 0.1f : 2.9f);
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
	static int t = 0;
	static int tt = 0;
	float v;
	float vf;

	// check if it is time to draw
	if (t++ == (int) (TICKSPERSEC * DRAWINTERVAL)) {
		t = 0;
		tickdraw = 1;
	}

	// if second is passed, flush pulsescount to output variable
	if (tt++ == TICKSPERSEC) {
		tt = 0;

		pulseslastsec = pulsecnt;
		pulsecnt = 0;
	}

	// get value from adc and put it into samples buffer
	v = getadcv(&hadc2) / (float) 0x00000fff * 3.0;

	if (bufpos >= BUFSIZE - WINSIZE) {
		bufpos = WINSIZE;
		memmove(buf, buf + (BUFSIZE - WINSIZE), WINSIZE);
	}

	buf[bufpos++] = v;

	// skip several first iterations of filtering.
	// should fill samples buffer before
	if (bufpos < WINSIZE)
		return;

	// perform median filtering to eliminate noise from comparator
	vf = medianfilter(buf + (bufpos - 1 - WINSIZE / 2), WINSIZE);

	// count PWM frequency and toggle indicating LED
	if (vf < 1.5f) {
		if (state != 1)
			pulsecnt++;
			
		state = 1;

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2,
			GPIO_PIN_RESET);
	}
	else {
		state = 0;

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2,
			GPIO_PIN_SET);
	}
}

// temperature reciever is a very simple ne555 based circuit where
// thermistor is one of the resistors that control frequency
float ne555freq2res(float freq)
{
	float r;
	float r1, c;

	r1 = 10000.0f;
	c = 0.00000022f;

	return ((2.88f - 2.0f * c * freq * r1)
		/ (4.0f * c * freq) / 10000.0f);
}

int main(void)
{	
	HAL_Init();

	systemclock_config();

	gpio_init();
	dma_init();
	adc1_init();
	adc2_init();
	tim2_init();
	display_init();

	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);

	displayset(DISPLAY_8BIT, DISPLAY_2LINE, DISPLAY_SMALLFONT);
	displayonoff(DISPLAY_ON, DISPLAY_NOCURSOR, DISPLAY_NOBLINK);
	displayclear();
	displayentry(DISPLAY_FORWARD, DISPLAY_NOSHIFTSCREEN);

	HAL_TIM_Base_Start_IT(&htim2);


	while (1) {
		char a[256];

		if (tickdraw) {
			float v;

			displayclear();
		
			tickdraw = 0;

			// take voltage from a pin with the thermistor
			// based divider attached
			v = getadcv(&hadc1) / (float) 0x00000fff;
			
			displaypos(0, 3);

			snprintf(a, 17, "%.4f C", voltagetotemp(v));
			displaystring(a);

/*
			displaypos(0, 3);
			snprintf(a, 17, "%lu", pulseslastsec);
			displaystring(a);
*/

			displaypos(1, 3);
			snprintf(a, 17, "%.4f C",
				res2temp(ne555freq2res((float) pulseslastsec)));
			displaystring(a);
		}
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
	__HAL_RCC_GPIOC_CLK_ENABLE();

	HAL_GPIO_WritePin(GPIOA,
		GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2
		| GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5
		| GPIO_PIN_6 | GPIO_PIN_7,
		GPIO_PIN_RESET);

	HAL_GPIO_WritePin(GPIOB,
		GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
		| GPIO_PIN_4,
		GPIO_PIN_RESET);

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2,
		GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2
		| GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5
		| GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1
		| GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}

static void dma_init(void)
{
	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_DMA2_CLK_ENABLE();

	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

	HAL_NVIC_SetPriority(DMA2_Channel1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Channel1_IRQn);
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

static void adc2_init(void)
{
	ADC_MultiModeTypeDef multimode = {0};
	ADC_ChannelConfTypeDef sConfig = {0};

	hadc2.Instance = ADC2;
	hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc2.Init.Resolution = ADC_RESOLUTION_12B;
	hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc2.Init.ContinuousConvMode = ENABLE;
	hadc2.Init.DiscontinuousConvMode = DISABLE;
	hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc2.Init.NbrOfConversion = 1;
	hadc2.Init.DMAContinuousRequests = ENABLE;
	hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc2.Init.LowPowerAutoWait = DISABLE;
	hadc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;

	if (HAL_ADC_Init(&hadc2) != HAL_OK)
		error_handler();

	multimode.Mode = ADC_MODE_INDEPENDENT;
	if (HAL_ADCEx_MultiModeConfigChannel(&hadc2,
			&multimode) != HAL_OK)
		error_handler();

	sConfig.Channel = ADC_CHANNEL_7;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.SamplingTime = ADC_SAMPLETIME_61CYCLES_5;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
		error_handler();
}

static void tim2_init(void)
{
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 71;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = TIMPERIOD - 1;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
		error_handler();

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
		error_handler();

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
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
