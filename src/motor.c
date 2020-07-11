#include "motor.h"
#include <stm32f4xx.h>
#include "gpio.h"
#include "power.h"
#include "digitalIO.h"


//motor direction pin states
typedef enum motor_dir_e
{
	MOTOR_DIR_FWD = 1,
	MOTOR_DIR_BWD = 2,
	MOTOR_DIR_BRAKE = 3
} motor_dir_t;


static gpio_pin_t pin_mode_01 = {GPIOC, 13};
static gpio_pin_t pin_mode_23 = {GPIOB, 5};

static gpio_pin_t pin_a[MOTOR_COUNT] = {
	{GPIOC, 9},
	{GPIOC, 11},
	{GPIOA, 8},
	#ifndef HW_VERSION
		{}};
		#error No HW_VERSION defined!
	#elif ((HW_VERSION == 3) || (HW_VERSION == 4) || (HW_VERSION == 10))
		{GPIOE, 7}};
	#elif (HW_VERSION == 2)
		{GPIOB, 10}};
	#else
		{}};
		#error Defined HW_VERSION is not supported by this firmware!
	#endif

static gpio_pin_t pin_b[MOTOR_COUNT] = {
	{GPIOC, 12},
	{GPIOC, 8},
	{GPIOC, 14},
	{GPIOC, 15}};

static gpio_pin_t fetOutput[4] = {
	{GPIOB, 7},
	{GPIOB, 6},
	{GPIOA, 12},
	{GPIOA, 11}};

//motor configuration
static motor_type_t type[MOTOR_COUNT] = {MOTOR_TYPE_DC, MOTOR_TYPE_DC, MOTOR_TYPE_DC, MOTOR_TYPE_DC}; //motor type
static uint8_t enc_in_a[MOTOR_COUNT] = {255, 255, 255, 255}; //input pin for enc_a
static uint8_t enc_in_b[MOTOR_COUNT] = {255, 255, 255, 255}; //input pin for enc_b

//motor set values
static motor_mode_t mode[MOTOR_COUNT] = {MOTOR_MODE_POWER, MOTOR_MODE_POWER, MOTOR_MODE_POWER, MOTOR_MODE_POWER}; //motor mode
static int16_t pow_vel_pos[MOTOR_COUNT] = {0, 0, 0, 0}; //power for power mode (0-1000) / target velocity for velocity mode (steps/s) / position for position mode (steps)

//measured values
static uint16_t counter_start[MOTOR_COUNT] = {0, 0, 0, 0};
static uint16_t counter_overflows[MOTOR_COUNT] = {0, 0, 0, 0};
static int16_t velocity[MOTOR_COUNT] = {0, 0, 0, 0};
static int16_t position[MOTOR_COUNT] = {0, 0, 0, 0};

//pid
static int16_t velocity_err_last[MOTOR_COUNT] = {0,0,0,0};
static int32_t velocity_err_integral[MOTOR_COUNT] = {0,0,0,0};
static int16_t position_err_last[MOTOR_COUNT] = {0,0,0,0};
static int32_t position_err_integral[MOTOR_COUNT] = {0,0,0,0};

//pwm
static motor_dir_t direction[MOTOR_COUNT] = {MOTOR_DIR_FWD, MOTOR_DIR_FWD, MOTOR_DIR_FWD, MOTOR_DIR_FWD}; //direction/braking
static uint16_t dutyCycle[MOTOR_COUNT] = {0, 0, 0, 0}; //0-MOTOR_MAX_POWER, anything above is interpreted as MOTOR_MAX_POWER


static uint8_t update_counter = 0;


void motor_init()
{
	//all motor pins to output
	gpio_pinCfg(pin_mode_01, MODE_OUT|OTYPE_PP|SPEED_LOW, 0);
	gpio_pinCfg(pin_mode_23, MODE_OUT|OTYPE_PP|SPEED_LOW, 0);
	uint8_t motor;
	for(motor=0; motor<MOTOR_COUNT; motor++) gpio_pinCfg(pin_a[motor], MODE_OUT|OTYPE_PP|SPEED_LOW, 0);
	for(motor=0; motor<MOTOR_COUNT; motor++) gpio_pinCfg(pin_b[motor], MODE_OUT|OTYPE_PP|SPEED_LOW, 0);

	gpio_pinSet(pin_mode_01, false); //motor driver mode 0
	gpio_pinSet(pin_mode_23, false); //motor driver mode 0

	//timer 2 init
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; //enable clock (84MHz)
	TIM2->PSC = 83; //prescaler = 84 --> 1MHz
	TIM2->ARR = MOTOR_MAX_POWER - 1; //auto-reload value = 999 --> 1kHz PWM frequency
	TIM2->CCMR1 |= (TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE); //channel 1+2 preload enable
	TIM2->CCMR2 |= (TIM_CCMR2_OC3PE | TIM_CCMR2_OC4PE); //channel 3+4 preload enable
	TIM2->DIER |= (TIM_DIER_UIE | TIM_DIER_CC1IE | TIM_DIER_CC2IE | TIM_DIER_CC3IE | TIM_DIER_CC4IE); //overflow & compare interrupts enable
	TIM2->CCR1 = dutyCycle[0]; //motor 0 dutyCycle 0-MOTOR_MAX_POWER
	TIM2->CCR2 = dutyCycle[1]; //motor 1 dutyCycle 0-MOTOR_MAX_POWER
	TIM2->CCR3 = dutyCycle[2]; //motor 2 dutyCycle 0-MOTOR_MAX_POWER
	TIM2->CCR4 = dutyCycle[3]; //motor 3 dutyCycle 0-MOTOR_MAX_POWER
	TIM2->CR1 |= TIM_CR1_CEN; //enable timer
	NVIC_EnableIRQ(TIM2_IRQn); //enable TIM2 global Interrupt
	NVIC_SetPriority(TIM2_IRQn, 0); //highest interrupt priority

	for(motor=0; motor<4; motor++) gpio_pinCfg(fetOutput[motor], MODE_OUT|OTYPE_PP|SPEED_LOW, 0);
}


void motor_update(uint8_t motor)
{
	switch(type[motor])
	{
		case MOTOR_TYPE_ENC:
			if(mode[motor] == MOTOR_MODE_VELOCITY) //TODO: improve velocity=0, maybe just full power against rotation direction?
			{
				int16_t velocity_err = pow_vel_pos[motor] - velocity[motor];
				int16_t velocity_err_diff = velocity_err - velocity_err_last[motor];
				velocity_err_integral[motor] += velocity_err;
				float power = (float)velocity_err * MOTOR_VELOCITY_P + (float)velocity_err_integral[motor] * MOTOR_VELOCITY_I + (float)velocity_err_diff * MOTOR_VELOCITY_D;
				if(power >= 0)
				{
					direction[motor] = MOTOR_DIR_FWD;
				}
				else
				{
					direction[motor] = MOTOR_DIR_BWD;
					power = -power;
				}
				if(power > MOTOR_MAX_POWER) power = MOTOR_MAX_POWER;
				dutyCycle[motor] = (uint16_t)(power + 0.5);
				break;
			}
//			else if(mode[motor] == MOTOR_MODE_POSITION) //TODO
//			{
//				int16_t position_err = pow_vel_pos[motor] - position[motor];
//				int16_t position_err_diff = position_err - position_err_last[motor];
//				position_err_integral[motor] += position_err;
//				float power = (float)position_err * MOTOR_POSITION_P + (float)position_err_integral[motor] * MOTOR_POSITION_I + (float)position_err_diff * MOTOR_POSITION_D;
//				if(power >= 0)
//				{
//					direction[motor] = MOTOR_DIR_FWD;
//				}
//				else
//				{
//					direction[motor] = MOTOR_DIR_BWD;
//					power = -power;
//				}
//				if(power > MOTOR_MAX_POWER) power = MOTOR_MAX_POWER;
//				dutyCycle[motor] = (uint16_t)(power + 0.5);
//				break;
//			}
			//else: fallthrough
		case MOTOR_TYPE_DC:
			if(mode[motor] == MOTOR_MODE_BRAKE) direction[motor] = MOTOR_DIR_BRAKE;
			if(pow_vel_pos[motor] >= 0)
			{
				if(mode[motor] != MOTOR_MODE_BRAKE) direction[motor] = MOTOR_DIR_FWD;
				dutyCycle[motor] = pow_vel_pos[motor];
			}
			else
			{
				if(mode[motor] != MOTOR_MODE_BRAKE) direction[motor] = MOTOR_DIR_BWD;
				dutyCycle[motor] = -pow_vel_pos[motor];
			}
			break;
		case MOTOR_TYPE_STEP:
			//TODO: stepper
			break;
	}

	switch(motor)
	{
		case 0:
			TIM2->CCR1 = dutyCycle[0];
			break;
		case 1:
			TIM2->CCR2 = dutyCycle[1];
			break;
		case 2:
			TIM2->CCR3 = dutyCycle[2];
			break;
		case 3:
			TIM2->CCR4 = dutyCycle[3];
			break;
	}
}


void TIM2_IRQHandler(void)
{
	if(TIM2->SR & TIM_SR_UIF) //counter overflow, start of pwm periods
	{
		TIM2->SR &= ~TIM_SR_UIF; //reset interrupt flag

		uint8_t motor;
		for(motor = 0; motor < 4; motor++) //for each motor
		{
			if(dutyCycle[motor] == 0)
			{
				gpio_pinSet(pin_a[motor],false);
				gpio_pinSet(pin_b[motor],false);
			}
			else
			{
				gpio_pinSet(pin_a[motor],(direction[motor] & 1));
				gpio_pinSet(pin_b[motor],(direction[motor] & 2));
			}
		}

		counter_overflows[0]++;
		counter_overflows[1]++;
		counter_overflows[2]++;
		counter_overflows[3]++;
		update_counter++;
		if(update_counter > 9)
		{
			update_counter = 0;
			motor_update(0);
			motor_update(1);
			motor_update(2);
			motor_update(3);
		}
	}

	if(TIM2->SR & TIM_SR_CC1IF) //compare 1 interrupt flag, end of motor 0 pwm period
	{
		TIM2->SR &= ~TIM_SR_CC1IF;
		if(dutyCycle[0] < MOTOR_MAX_POWER)
		{
			gpio_pinSet(pin_a[0],false);
			gpio_pinSet(pin_b[0],false);
		}
	}
	if(TIM2->SR & TIM_SR_CC2IF) //compare 2 interrupt flag, end of motor 1 pwm period
	{
		TIM2->SR &= ~TIM_SR_CC2IF;
		if(dutyCycle[1] < MOTOR_MAX_POWER)
		{
			gpio_pinSet(pin_a[1],false);
			gpio_pinSet(pin_b[1],false);
		}
	}
	if(TIM2->SR & TIM_SR_CC3IF) //compare 3 interrupt flag, end of motor 2 pwm period
	{
		TIM2->SR &= ~TIM_SR_CC3IF;
		if(dutyCycle[2] < MOTOR_MAX_POWER)
		{
			gpio_pinSet(pin_a[2],false);
			gpio_pinSet(pin_b[2],false);
		}
	}
	if(TIM2->SR & TIM_SR_CC4IF) //compare 4 interrupt flag, end of motor 3 pwm period
	{
		TIM2->SR &= ~TIM_SR_CC4IF;
		if(dutyCycle[3] < MOTOR_MAX_POWER)
		{
			gpio_pinSet(pin_a[3],false);
			gpio_pinSet(pin_b[3],false);
		}
	}
}


void exti_handler(uint8_t pin) //TODO improve, use full resolution, filter velocity
{
	uint8_t m = 0xFF;
	if(pin == enc_in_a[0]) m = 0;
	else if(pin == enc_in_a[1]) m = 1;
	else if(pin == enc_in_a[2]) m = 2;
	else if(pin == enc_in_a[3]) m = 3;

	if(m == 0xFF) return;

	if(digitalIO_getState(enc_in_a[m])) return; //only falling edge of a

	uint32_t us = counter_overflows[m] * 1000 + TIM2->CNT - counter_start[m];
	counter_start[m] = TIM2->CNT;
	counter_overflows[m] = 0;
	if(digitalIO_getState(enc_in_b[m]))
	{
		velocity[m] = -(uint16_t)(1000000.0 / (float)us + 0.5);
		position[m]--;
	}
	else
	{
		velocity[m] = (uint16_t)(1000000.0 / (float)us + 0.5);
		position[m]++;
	}
}

void EXTI9_5_IRQHandler(void)
{
	if(EXTI->PR & (1<<7))
	{
		EXTI->PR |= (1<<7);
		exti_handler(7);
	}
	if(EXTI->PR & (1<<6))
	{
		EXTI->PR |= (1<<6);
		exti_handler(6);
	}
	if(EXTI->PR & (1<<5))
	{
		EXTI->PR |= (1<<5);
		exti_handler(5);
	}
}
void EXTI4_IRQHandler(void)
{
	if(EXTI->PR & (1<<4))
	{
		EXTI->PR |= (1<<4);
		exti_handler(4);
	}
}
void EXTI3_IRQHandler(void)
{
	if(EXTI->PR & (1<<3))
	{
		EXTI->PR |= (1<<3);
		exti_handler(3);
	}
}
void EXTI2_IRQHandler(void)
{
	if(EXTI->PR & (1<<2))
	{
		EXTI->PR |= (1<<2);
		exti_handler(2);
	}
}
void EXTI1_IRQHandler(void)
{
	if(EXTI->PR & (1<<1))
	{
		EXTI->PR |= (1<<1);
		exti_handler(1);
	}
}
void EXTI0_IRQHandler(void)
{
	if(EXTI->PR & (1<<0))
	{
		EXTI->PR |= (1<<0);
		exti_handler(0);
	}
}


void motor_configure(uint8_t motor, motor_type_t motorType, uint8_t encoder_a, uint8_t encoder_b)
{
	if(motor >= MOTOR_COUNT) return;
	if(encoder_a >= DIGITAL_COUNT) return;
	if(encoder_b >= DIGITAL_COUNT) return;
	if(encoder_a == encoder_b) return;

	if(type[motor] == MOTOR_TYPE_ENC)
	{
		digitalIO_freePin(enc_in_a[motor]);
		digitalIO_freePin(enc_in_b[motor]);
		EXTI->IMR &= ~(1 << enc_in_a[motor]);
		EXTI->IMR &= ~(1 << enc_in_b[motor]);
	}

	switch(motorType)
	{
		case MOTOR_TYPE_DC:
			break;
		case MOTOR_TYPE_ENC:
			digitalIO_usePinEnc(encoder_a);
			digitalIO_usePinEnc(encoder_b);
			enc_in_a[motor] = encoder_a;
			enc_in_b[motor] = encoder_b;
			EXTI->IMR |= (1 << enc_in_a[motor]);
			EXTI->IMR |= (1 << enc_in_b[motor]);
			break;
		case MOTOR_TYPE_STEP:
			//TODO: stepper
			break;
	}

	type[motor] = motorType;
	mode[motor] = MOTOR_MODE_POWER; //TODO: not for stepper
	pow_vel_pos[motor] = 0;
	motor_update(motor);
}


void motor_set(uint8_t motor, motor_mode_t motorMode, int16_t power_velocity)
{
	if((motor >= MOTOR_COUNT + 4) || (motorMode > 3)) return;

	if(motor > 3)
	{
		if(power_velocity > 0) gpio_pinSet(fetOutput[motor - 3], true);
		else gpio_pinSet(fetOutput[motor - 3], false);
		return;
	}

	if(type[motor] == MOTOR_TYPE_DC && !(motorMode == MOTOR_MODE_POWER || motorMode == MOTOR_MODE_BRAKE)) return;
	if(type[motor] == MOTOR_TYPE_STEP && motorMode == MOTOR_MODE_POWER && power_velocity != 0) return;
	if(power_getEmergencyStop() && (power_velocity != 0)) return; //TODO: allow only power and brake with power_velocity=0

	mode[motor] = motorMode;
	if(motorMode == MOTOR_MODE_POWER || motorMode == MOTOR_MODE_BRAKE)
	{
		if(power_velocity > MOTOR_MAX_POWER) power_velocity = MOTOR_MAX_POWER;
		else if(power_velocity < -MOTOR_MAX_POWER) power_velocity = -MOTOR_MAX_POWER;
	}
	pow_vel_pos[motor] = power_velocity;

	if(motorMode != MOTOR_MODE_VELOCITY) velocity_err_integral[motor] = 0;

	motor_update(motor);
}


void motor_positional(uint8_t motor, motor_mode_t motorMode, int16_t power_velocity, motor_pos_done_mode_t doneMode, bool relative, int32_t position)
{
	//TODO
}


void motor_servo(uint8_t motor, int16_t velocity_max, bool relative, int32_t position)
{
	//TODO
}


void motor_allOff()
{
	motor_set(0, MOTOR_MODE_POWER, 0);
	motor_set(1, MOTOR_MODE_POWER, 0);
	motor_set(2, MOTOR_MODE_POWER, 0);
	motor_set(3, MOTOR_MODE_POWER, 0);
}
