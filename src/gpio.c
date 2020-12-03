#include "gpio.h"


void gpio_init()
{
	RCC->AHB1ENR |=	RCC_AHB1ENR_GPIOAEN | //enable clock for GPIOA
					RCC_AHB1ENR_GPIOBEN | //enable clock for GPIOB
					RCC_AHB1ENR_GPIOCEN | //enable clock for GPIOC
					RCC_AHB1ENR_GPIODEN | //enable clock for GPIOD
					RCC_AHB1ENR_GPIOEEN;  //enable clock for GPIOE
}


void gpio_pinCfg(gpio_pin_t pin, gpio_cfg_t cfg, uint8_t af) //TODO: add parameter error checking
{
	pin.port->MODER &= ~(0x3 << (2*pin.pinNr));
	pin.port->MODER |= ((cfg & 0x3) << (2*pin.pinNr));

	pin.port->OTYPER &= ~(0x1 << (pin.pinNr));
	pin.port->OTYPER |= (((cfg >> 2) & 0x1) << (pin.pinNr));

	pin.port->OSPEEDR &= ~(0x3 << (2*pin.pinNr));
	pin.port->OSPEEDR |= (((cfg >> 3) & 0x3) << (2*pin.pinNr));

	pin.port->PUPDR &= ~(0x3 << (2*pin.pinNr));
	pin.port->PUPDR |= (((cfg >> 5) & 0x3) << (2*pin.pinNr));

	if(pin.pinNr < 8)
	{
		pin.port->AFR[0] &= ~(0xF << (4*pin.pinNr));
		pin.port->AFR[0] |= (af << (4*pin.pinNr));
	}
	else
	{
		pin.port->AFR[1] &= ~(0xF << (4*(pin.pinNr - 8)));
		pin.port->AFR[1] |= (af << (4*(pin.pinNr - 8)));
	}
}


inline void gpio_pinSet(gpio_pin_t pin, bool state)
{
	if(state) pin.port->BSRR |= (1 << pin.pinNr);
	else pin.port->BSRR |= (1 << (pin.pinNr + 16));
}


inline bool gpio_pinGet(gpio_pin_t pin)
{
	return pin.port->IDR & (1 << pin.pinNr);
}
