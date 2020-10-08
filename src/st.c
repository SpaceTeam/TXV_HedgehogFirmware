#include "st.h"
#include <stm32f4xx.h>
#include "gpio.h"

static const gpio_pin_t pin_clk_out = {GPIOB, 12};
static const gpio_pin_t pin_dat_in[3] = {{GPIOB, 13}, {GPIOB, 15}, {GPIOB, 14}};

static volatile int32_t hx711_value[3] = {0,0,0};


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
	NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn); //enable TIM9 global Interrupt
	NVIC_SetPriority(TIM1_BRK_TIM9_IRQn, 0); //interrupt priority
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
				if(counter == 25) //done
				{
					for(i=0; i<3; i++)
					{
						hx711_value[i] = hx711_value_temp[i];
						hx711_value_temp[i] = 0;
					}
					counter = -1;
				}
				else if(counter == 24) //nearly done
				{
					counter++;
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
