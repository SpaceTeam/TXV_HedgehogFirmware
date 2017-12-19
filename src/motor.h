#ifndef MOTOR_H_
#define MOTOR_H_


#include <stdint.h>
#include <stdbool.h>


#define MOTOR_COUNT 4
#define MOTOR_MAX_POWER 1000

//motor modes, in coast mode power setting has no influence
#define MOTOR_MODE_COAST 0
#define MOTOR_MODE_BREAK 1
#define MOTOR_MODE_FORWARD 2
#define MOTOR_MODE_REVERSE 3

//maximum average motor voltage
#define MOTOR_MAX_VOLTAGE 6.0


//initializes timer 3 and outputs for 4 motors
void motor_init();

//sets motor mode, motor: 0-3, mode: see defines above
void motor_setMode(uint8_t motor, uint8_t motorMode);

//sets motor power (battery voltage compensated), motor: 0-3, power: 0-1000
void motor_setPower(uint8_t motor, uint16_t power);

//updates duty cycle according to setPower and battery voltage, motor: 0-3
void motor_updateDutyCycle(uint8_t motor);

//all motors to coast and 0 power
void motor_allOff();

#endif /* MOTOR_H_ */
