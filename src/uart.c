#include "uart.h"
#include "gpio.h"
#include <stm32f4xx.h>


static const gpio_pin_t pinTxD = {GPIOA, 9};
static const gpio_pin_t pinRxD = {GPIOA, 10};

static ringbuffer_t uart_rx_rb;
static ringbuffer_t uart_tx_rb;
volatile static uint8_t rx_buffer[512];
volatile static uint8_t tx_buffer[512];


static void uart_startFifoTransmit();


void uart_init()
{
	ringbuffer_init(&uart_rx_rb, rx_buffer, sizeof(rx_buffer), NULL);
	ringbuffer_init(&uart_tx_rb, tx_buffer, sizeof(tx_buffer), &uart_startFifoTransmit);

	gpio_pinCfg(pinTxD, MODE_AF|OTYPE_PP|SPEED_LOW, 7);
	gpio_pinCfg(pinRxD, MODE_AF, 7);

	RCC->APB2ENR |= RCC_APB2ENR_USART1EN; //enable UART1 clock (84MHz)
//	USART1->BRR = (45<<4) | 9;//Baudrate = 115200, 84MHz / (2 * 8 * 115200) ~ 45.5625, 0.5625 * 16 = 9
//	USART1->BRR = (22<<4) | 13;//Baudrate = 230400, 84MHz / (2 * 8 * 230400) ~ 22.8125, 0.8125 * 16 = 13
//	USART1->BRR = (5<<4) | 11;//Baudrate = 921600, 84MHz / (2 * 8 * 921600) ~ 5.6875, 0.6875 * 16 = 11
	USART1->BRR = (11<<4) | 6;//Baudrate = 460800, 84MHz / (2 * 8 * 460800) ~ 11.375, 0.375 * 16 = 6
	USART1->CR1 |= USART_CR1_RXNEIE; //rx not empty interrupt enable
	USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE); //enable Tx & Rx
	NVIC_EnableIRQ(USART1_IRQn); //enable UART1 global interrupts
	NVIC_SetPriority(USART1_IRQn, 7); //medium interrupt priority
	USART1->CR1 |= USART_CR1_UE; //enable UART
}


void USART1_IRQHandler(void)
{
	if(USART1->SR & USART_SR_TXE) //tx data register empty TODO: also check for TXEIE enabled
	{
		if(ringbuffer_pop(&uart_tx_rb, (uint8_t*)(&USART1->DR)) == RB_ERROR_UNDERFLOW) //send data if more data to send available, clears TXE flag
			USART1->CR1 &= ~USART_CR1_TXEIE; //no more data, transfer done --> disable TXE interrupt
	}

	if(USART1->SR & USART_SR_RXNE) //Rx data register not empty
	{
		USART1->SR &= ~USART_SR_RXNE; //clear interrupt flag to make sure it is cleared even when rx_rb is full and data doesn't get read in line below
		ringbuffer_push(&uart_rx_rb, USART1->DR); //write new data to buffer TODO: add buffer overflow error (some beep?)
	}

	//TODO: check for and clear ORE flag, error handling (some beep?)
}


static void uart_startFifoTransmit()
{
	if(!(USART1->CR1 & USART_CR1_TXEIE)) //if TXE interrupt disabled --> no transfer in progress
	{
		ringbuffer_pop(&uart_tx_rb, (uint8_t*)(&USART1->DR)); //send first byte to start transmission TODO: add buffer underflow error (some beep?)
		USART1->CR1 |= USART_CR1_TXEIE; //enable TXE interrupt
	}
}


ringbuffer_t *uart_getRxRingbuffer()
{
	return &uart_rx_rb;
}

ringbuffer_t *uart_getTxRingbuffer()
{
	return &uart_tx_rb;
}
