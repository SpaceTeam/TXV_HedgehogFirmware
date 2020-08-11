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
	USART1->BRR = (22<<4) | 12;//Baudrate = 115200, 84MHz / (2 * 8 * 230400) = 22.786, 0.786 * 15 = ~12
	USART1->CR1 |= (USART_CR1_TCIE | USART_CR1_RXNEIE); //transmission complete & Rx not empty interrupts enable
	USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE); //enable Tx & Rx
	NVIC_EnableIRQ(USART1_IRQn); //enable UART1 global interrupts
	NVIC_SetPriority(USART1_IRQn, 7); //medium interrupt priority
	USART1->CR1 |= USART_CR1_UE; //enable UART
}


void USART1_IRQHandler(void)
{
	if(USART1->SR & USART_SR_TC) //transmission complete (also fires after reset, not a problem)
	{
		USART1->SR &= ~USART_SR_TC; //clear interrupt flag
		ringbuffer_pop(&uart_tx_rb, (uint8_t*)(&USART1->DR)); //send data if more data to send available
	}

	if(USART1->SR & USART_SR_RXNE) //Rx data register not empty
	{
		USART1->SR &= ~USART_SR_RXNE; //clear interrupt flag
		ringbuffer_push(&uart_rx_rb, USART1->DR); //write new data to buffer TODO: add buffer overflow error
	}
}


static void uart_startFifoTransmit()
{
	if(USART1->SR & USART_SR_TXE) //if Tx data register is empty TODO: and not busy?
	{
		NVIC_DisableIRQ(USART1_IRQn); //disable UART1 global interrupts TODO: is it possible to make this interrupt-safe?
		ringbuffer_pop(&uart_tx_rb, (uint8_t*)(&USART1->DR)); //send first byte to start transmission TODO: add buffer underflow error
		NVIC_EnableIRQ(USART1_IRQn); //enable UART1 global interrupts
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
