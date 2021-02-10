#include "st.h"
#include <stm32f4xx.h>
#include "gpio.h"
#include <math.h>
#include "servo.h"
#include "adc.h"
#include "systick.h"

static const gpio_pin_t pin_clk_out = {GPIOB, 12};
static const gpio_pin_t pin_dat_in[3] = {{GPIOB, 13}, {GPIOB, 15}, {GPIOB, 14}};

static volatile int32_t hx711_value[3] = {0,0,0};

static volatile bool pressureRegEnabled = true;

static float pressureThreshold = PRESSURE_THRESHOLD;
static float pressureHysteresis = PRESSURE_HYSTERESIS;



void st_init(void)
{
	//3x HX711

	gpio_pinCfg(pin_clk_out, MODE_OUT|OTYPE_PP|SPEED_LOW, 0);
	gpio_pinCfg(pin_dat_in[0], MODE_IN|PULL_FL, 0);
	gpio_pinCfg(pin_dat_in[1], MODE_IN|PULL_FL, 0);
	gpio_pinCfg(pin_dat_in[2], MODE_IN|PULL_FL, 0);
	gpio_pinSet(pin_clk_out, false);

	RCC->APB2ENR |= RCC_APB2ENR_TIM9EN; //enable clock (84MHz)
	TIM9->PSC = 83; //prescaler = 84 --> 1MHz
	TIM9->ARR = 49; //auto-reload value = 49 --> 20kHz interrupt frequency
	TIM9->DIER |= (TIM_DIER_UIE); //overflow interrupt enable
	TIM9->CR1 |= TIM_CR1_CEN; //enable timer
	NVIC_SetPriority(TIM1_BRK_TIM9_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 3));
	NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn); //enable TIM9 global Interrupt
	st_setPressurethreshold(PRESSURE_THRESHOLD, PRESSURE_HYSTERESIS);
}


int32_t st_getThrust(uint8_t i)
{
	return hx711_value[i];
}


void TIM1_BRK_TIM9_IRQHandler(void) //20kHz
{
	if(TIM9->SR & TIM_SR_UIF) // 3x HX711
	{
		TIM9->SR &= ~TIM_SR_UIF; //reset interrupt flag

		static int8_t counter = -1;
		static int32_t hx711_value_temp[3] = {0,0,0};
		uint8_t i;

		if(counter == -1) //wait for conversion complete
		{
			if(!(gpio_pinGet(pin_dat_in[0]) || gpio_pinGet(pin_dat_in[1]) || gpio_pinGet(pin_dat_in[2]))) //all data pins low --> all conversions done
			{
				counter = 0;
			}
		}
		else //data transmission
		{
			if(gpio_pinGet(pin_clk_out)) //clock has been high --> read data, set low again
			{
				if(counter == 24) //done
				{
					for(i=0; i<3; i++)
					{
						hx711_value[i] = hx711_value_temp[i];
						hx711_value_temp[i] = 0;
					}
					counter = -1;
				}
				else
				{
					for(i=0; i<3; i++)
					{
						hx711_value_temp[i] = (hx711_value_temp[i] << 1);
						hx711_value_temp[i] |= gpio_pinGet(pin_dat_in[i]);
					}
					counter++;
				}
				gpio_pinSet(pin_clk_out, false);
			}
			else //clock has been low --> set high
			{
				gpio_pinSet(pin_clk_out, true);
			}
		}
	}
}



void st_enablePressureControl(uint8_t enable)
{
	pressureRegEnabled = enable;
}

void st_setPressurethreshold(uint8_t threshold, uint8_t hysteresis) // bar, bar/10
{
	pressureThreshold = (float)threshold;
	pressureHysteresis = (float)hysteresis / 10.0;
}


void st_loop(void)
{
	static uint32_t systick_last = 0;
	static uint32_t systick_lastMovement = 0;
	static float pressure_old = -100;
	static float threshold;

	if(systick_getUptime() > systick_last) //1kHz
	{
		float dt = systick_getUptime() - systick_last; //time interval
		systick_last = systick_getUptime(); //current time
		static float valvePercentage = 1; //init valve percentage

		float pressure = (float)adc_getAnalogInput(2) * 0.023497 - 8.74088; //get pressure in bar
		static float filt_pressure = -100;
		filt_pressure = filt_pressure * (0.999) + pressure * 0.001;
		float dp = filt_pressure - pressure_old; //change in pressure
	    float gradient = dp / dt; //change in pressure over time interval
		static float gradient_filt = 0;
		gradient_filt = gradient_filt * 0.99 + gradient * 0.01;
		static uint16_t grad_pos_count = 0;
		if(gradient_filt >= -0.001)
		{
			grad_pos_count++;
		}
		else
		{
			grad_pos_count = 0;
		}
		if(pressure > threshold && pressureRegEnabled) //if pressure too high and enabled
		{
			threshold = pressureThreshold - pressureHysteresis;
			if(grad_pos_count > 100)
			{
				if(valvePercentage < 100.0)
				{
					valvePercentage += ((valvePercentage < 25.0) ? 0.1 : 0.02); //1 : ? to full open
					systick_lastMovement = systick_last; //time stamp last movement
				}
			}
		}
		else // pressure < threshold or pressureRegEnabled = false
		{
			threshold = pressureThreshold;
			if(valvePercentage != 0)
			{
				valvePercentage = 0;
				systick_lastMovement = systick_last;
			}
		}
		servo_setOntime(3, (uint16_t)(1040 + valvePercentage / 100.0 * 950.0 + 0.5) * 2);

		servo_setEnabled(3, (systick_lastMovement  + 2000 > systick_last)); //disable servo after 2seconds
		pressure_old = filt_pressure; //set old pressure to current pressure
	}
}
