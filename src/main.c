#include <stdint.h>
#include "systick.h"
#include "power.h"
#include "gpio.h"
#include "digitalIO.h"
#include "output.h"
#include "servo.h"
#include "adc.h"
#include "motor.h"
#include "uart.h"
#include "hcp.h"
#include "imu.h"
#include "st.h"


#define SHUTDOWN_TIMEOUT 20


bool rpi_was_active = false;

//interrupt priorities (preemtive/sub):
//uart 0/0
//adc (filter update) 1/5
//imu (spi) 1/4
//digitalIO (ext. int) 1/0
//motor (pwm) 1/1
//output (led pwm) 1/2
//st (thrust read) 1/3
//systick 1/6

//TODO: make some variables constant or volatile
//TODO: use enums instead of defines for modes and status returns
//TODO: error checking everywhere the ringbuffer functions are used
//TODO: hcp handlers should use error codes from set-functions
int main()
{
	NVIC_SetPriorityGrouping(1);
	systick_init();
	gpio_init();
	adc_init();
	power_init();
	output_init();

	systick_busyWait(systick_timeToTicks(0, 0, 1, 0));

	power_on();
	powerLed(POWER_LED_MODE_BOOT);

	speaker_startup();

	power_regRpiEnable(true);
	power_regMsEnable(true);

	imu_init();
	digitalIO_init();
	servo_init();
	motor_init();
	st_init();
	uart_init();
	hcp_init();

	while(!power_getShutdown() && !power_getImmidiateShutdown())
	{
		if(power_getRPiActive())
		{
			rpi_was_active = true;
			powerLed(POWER_LED_MODE_ON);
		}
		if(rpi_was_active && (!power_getRPiActive())) break;

		hcp_update();
		adc_update();
	}

	led0(false);
	led1(false);
	speaker_setFrequency(0);
	motor_allOff();
	servo_allOff();

	hcp_sendShutdown();
	powerLed(POWER_LED_MODE_SHUTDOWN);
	speaker_shutdown();

	power_regMsEnable(false);

	uint64_t timeout = systick_getUptime() + systick_timeToTicks(0, 0, SHUTDOWN_TIMEOUT, 0);
	while((power_getRPiActive()) && (systick_getUptime() < timeout && !power_getImmidiateShutdown())); // wait for active pin to go low or timeout or button kept pressed after beep
	if(rpi_was_active && !power_getImmidiateShutdown()) systick_busyWait(systick_timeToTicks(0, 0, 8, 0)); // wait for RPi fully off

	power_regRpiEnable(false);
	powerLed(POWER_LED_MODE_OFF);
	power_off();

	while(1);

	return 0;
}


void HardFault_Handler(void)
{
	speaker_setFrequency(500);
	while(1);
}
