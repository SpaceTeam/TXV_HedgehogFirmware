#include "systick.h"
#include <stm32f4xx.h>
#include "power.h"
#include "output.h"
#include "imu.h"


static volatile uint64_t systick_count = 0;


void systick_init()
{
	NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 6));
	SysTick_Config(SystemCoreClock / 1000); //1ms period
	systick_count = 0;
}


inline uint64_t systick_getUptime()
{
	return systick_count;
}


void systick_busyWait(uint32_t ticks)
{
	uint64_t end = systick_count + ticks;
	while(systick_count < end);
}


inline uint32_t systick_timeToTicks(uint16_t h, uint8_t m, uint8_t s, uint16_t ms)
{
	return (uint32_t)(3600000*h + 60000*m + 1000*s + ms);
}


void SysTick_Handler()
{
	systick_count++;
	power_update();
	output_update();
	imu_update();
}
