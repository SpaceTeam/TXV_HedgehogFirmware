#include "digitalIO.h"
#include "gpio.h"

static gpio_pin_t digitalPin[DIGITAL_COUNT] = {
	{GPIOD, 0},
	{GPIOD, 1},
	{GPIOD, 2},
	{GPIOD, 3},
	{GPIOD, 4},
	{GPIOD, 5},
	{GPIOD, 6},
	{GPIOD, 7},
	{GPIOE, 15},
	{GPIOE, 14},
	{GPIOE, 13},
	{GPIOE, 12},
	{GPIOE, 11},
	{GPIOE, 10},
	{GPIOE, 0},
	{GPIOE, 1}};


static pin_mode_t mode[DIGITAL_COUNT] = {
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING,
	PIN_MODE_IN_FLOATING};

static bool pinInUse[DIGITAL_COUNT] = {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};


void digitalIO_init()
{
	uint8_t i;
	for(i=8; i<16; i++) digitalIO_setMode(i, mode[i]);

	//inputs 0-7 as external interrupts 0-7 for encoder inputs, both edges
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	SYSCFG->EXTICR[0] |= (3<<0) | (3<<4) | (3<<8) | (3<<12);
	SYSCFG->EXTICR[1] |= (3<<0) | (3<<4) | (3<<8) | (3<<12);
	EXTI->RTSR |= 0x00FF;
	EXTI->FTSR |= 0x00FF;
	EXTI->PR |= 0x00FF;
	NVIC_SetPriority(EXTI9_5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
	NVIC_SetPriority(EXTI4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
	NVIC_SetPriority(EXTI3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
	NVIC_SetPriority(EXTI2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
	NVIC_SetPriority(EXTI1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
	NVIC_SetPriority(EXTI0_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI4_IRQn);
	NVIC_EnableIRQ(EXTI3_IRQn);
	NVIC_EnableIRQ(EXTI2_IRQn);
	NVIC_EnableIRQ(EXTI1_IRQn);
	NVIC_EnableIRQ(EXTI0_IRQn);
}


void digitalIO_setMode(uint8_t pin, pin_mode_t pinMode)
{
	if(pin >= DIGITAL_COUNT) return;
	if(pinInUse[pin] && pinMode == PIN_MODE_OUT) return;

	mode[pin] = pinMode;

	if(pinMode == PIN_MODE_IN_FLOATING) gpio_pinCfg(digitalPin[pin], MODE_IN|PULL_FL, 0);
	else if(pinMode == PIN_MODE_IN_PULLUP) gpio_pinCfg(digitalPin[pin], MODE_IN|PULL_PU, 0);
	else if(pinMode == PIN_MODE_IN_PULLDOWN) gpio_pinCfg(digitalPin[pin], MODE_IN|PULL_PD, 0);
	else if(pinMode == PIN_MODE_OUT) gpio_pinCfg(digitalPin[pin], MODE_OUT|OTYPE_PP|SPEED_LOW, 0);
}


pin_mode_t digitalIO_getMode(uint8_t pin)
{
	return mode[pin];
}


bool digitalIO_getState(uint8_t pin)
{
	if(pin >= DIGITAL_COUNT) return 0;
	return gpio_pinGet(digitalPin[pin]);
}


void digitalIO_setState(uint8_t pin, bool state)
{
	if(pin >= DIGITAL_COUNT) return;
	gpio_pinSet(digitalPin[pin], state);
}


void digitalIO_usePinEnc(uint8_t pin)
{
	pinInUse[pin] = true;
	digitalIO_setMode(pin, PIN_MODE_IN_PULLUP);
}

void digitalIO_freePin(uint8_t pin)
{
	pinInUse[pin] = false;
}

bool digitalIO_isInUse(uint8_t pin)
{
	return pinInUse[pin];
}
