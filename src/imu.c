#include "imu.h"
#include <stm32f4xx.h>
#include "gpio.h"
#include <math.h>


static const gpio_pin_t pin_imu_ncs = {GPIOE, 4};
static const gpio_pin_t pin_imu_sck = {GPIOE, 2};
static const gpio_pin_t pin_imu_mosi = {GPIOE, 6};
static const gpio_pin_t pin_imu_miso = {GPIOE, 5};
static const gpio_pin_t pin_imu_int = {GPIOE, 3};


static uint8_t read_count = 0;
static uint8_t imu_data[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static int16_t acc[3] = {0, 0, 0};
static int16_t rate[3] = {0, 0, 0};
static int16_t pose[3] = {0, 0, 0};


static void imu_writeRegister(uint8_t reg, uint8_t value)
{
	gpio_pinSet(pin_imu_ncs, false);
	while(SPI4->SR & SPI_SR_BSY); //wait until not busy
	SPI4->DR = WRITE | reg; //write and address
	while(!(SPI4->SR & SPI_SR_TXE)); //while tx buffer not empty
	SPI4->DR = value; //value
	while(!(SPI4->SR & SPI_SR_TXE)); //while tx buffer not empty
	while(SPI4->SR & SPI_SR_BSY); //while busy
	gpio_pinSet(pin_imu_ncs, true);
}

static uint8_t imu_readRegister(uint8_t reg)
{
	uint8_t value = 0;
	gpio_pinSet(pin_imu_ncs, false);
	SPI4->DR = READ | reg;
	while(!(SPI4->SR & SPI_SR_RXNE)); //while rx buffer empty
	SPI4->DR = 0x00;
	while(!(SPI4->SR & SPI_SR_RXNE)); //while rx buffer empty
	value = SPI4->DR;
	gpio_pinSet(pin_imu_ncs, true);
	return value;
}


void SPI4_IRQHandler(void)
{
	if(SPI4->SR & SPI_SR_RXNE) //new rx data available
	{
		imu_data[read_count] = SPI4->DR; //read rx data to clear flag

		if(read_count < 14)
		{
			read_count++;
			SPI4->DR = 0; //start next transmission
		}
		else //done
		{
			gpio_pinSet(pin_imu_ncs, true);
		}
	}
}


void imu_init()
{
	gpio_pinCfg(pin_imu_ncs, MODE_OUT|OTYPE_PP|SPEED_LOW, 0);
	gpio_pinSet(pin_imu_ncs, true);
	gpio_pinCfg(pin_imu_sck, MODE_AF|OTYPE_PP|SPEED_LOW, 5);
	gpio_pinCfg(pin_imu_mosi, MODE_AF|OTYPE_PP|SPEED_LOW, 5);
	gpio_pinCfg(pin_imu_miso, MODE_AF|PULL_PD, 5);
	gpio_pinCfg(pin_imu_int, MODE_IN|PULL_PU, 0);

	//SPI4 init
	RCC->APB2ENR |= RCC_APB2ENR_SPI4EN; //clock enabled
	SPI4->CR1 |= SPI_CR1_BR; //clock = fPCLK/256 = ~333KhZ
	SPI4->CR1 |= SPI_CR1_MSTR; //master mode
	SPI4->CR1 |= SPI_CR1_CPOL; //clk high when idle
	SPI4->CR1 |= SPI_CR1_CPHA; //first data capture on second edge
	SPI4->CR2 |= SPI_CR2_SSOE; //SS output enable
	SPI4->CR1 |= SPI_CR1_SPE; //enable spi

	//IMU init TODO improve: sample rate, averaging
	imu_writeRegister(27, 0x10); //gyro range ±1000dps
	imu_writeRegister(28, 0x08); //accel range ±4g
	imu_writeRegister(107, 0); //POWER_MANAGEMENT_1, on, 20MHz internal oscillator

	imu_data[0] = SPI4->DR; //read rx data to clear flag
	SPI4->CR2 |= SPI_CR2_RXNEIE; //rx not empty interrupt enable
	NVIC_SetPriority(SPI4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 4));
	NVIC_EnableIRQ(SPI4_IRQn); //enable SPI4 global Interrupt
}


void imu_update()
{
	acc[0] = ((imu_data[3] << 8) | imu_data[4]); //x
	acc[1] = -((imu_data[1] << 8) | imu_data[2]); //y
	acc[2] = ((imu_data[5] << 8) | imu_data[6]); //z
	rate[0] = ((imu_data[11] << 8) | imu_data[12]); //x
	rate[1] = -((imu_data[9] << 8) | imu_data[10]); //y
	rate[2] = ((imu_data[13] << 8) | imu_data[14]); //z

	//complementary filter for pose
	uint8_t i;
	for(i=0; i<3; i++) pose[i] += (rate[i] * (1000.0 / 360.0) * 0.001); //integrate rate
	uint32_t acc_magn = sqrt(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]); //magnitude of acceleration
	if(6554 < acc_magn && acc_magn < 9830) //0.8g < acc_magn < 1.2g, no strong disturbance --> correct pose drift with acceleration data
	{
		int16_t acc_angle[3];
		acc_angle[0] = (int16_t)(atan2f(acc[1], acc[2]) * 32767.0 / M_PI); //x
		acc_angle[1] = -(int16_t)(atan2f(acc[0], acc[2]) * 32767.0 / M_PI); //y
		acc_angle[2] = (int16_t)(atan2f(acc[0], acc[1]) * 32767.0 / M_PI); //z

		for(i=0; i<3; i++) pose[i] = 0.99 * pose[i] + 0.01 * acc_angle[i];
	}

	//start spi read
	while(SPI4->SR & SPI_SR_BSY); //wait until not busy
	read_count = 0;
	gpio_pinSet(pin_imu_ncs, false);
	SPI4->DR = READ | 59;
}


int16_t imu_getRateX()
{
	return rate[0];
}

int16_t imu_getRateY()
{
	return rate[1];
}

int16_t imu_getRateZ()
{
	return rate[2];
}


int16_t imu_getAccelX()
{
	return acc[0];
}

int16_t imu_getAccelY()
{
	return acc[1];
}

int16_t imu_getAccelZ()
{
	return acc[2];
}


int16_t imu_getPoseX()
{
	return pose[0];
}

int16_t imu_getPoseY()
{
	return pose[1];
}

int16_t imu_getPoseZ()
{
	return pose[2];
}
