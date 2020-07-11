#include <stdbool.h>
#include <stdlib.h>
#include "hcp_handler.h"
#include "hcp_opcodes.h"
#include "ringbuffer.h"
#include "motor.h"
#include "power.h"


void hcp_handler_motor(hcp_conn_t conn, uint8_t opcode, size_t payloadLength) //TODO: add check: for stepper only velocity and brake modes are allowed, check for emergency active everywhere
{
	if(opcode == HCP_MOTOR)
	{
		uint8_t port;
		if(ringbuffer_pop(conn.rxBuffer, &port)) return;

		uint8_t mode;
		if(ringbuffer_pop(conn.rxBuffer, &mode)) return;

		uint8_t pv_h;
		if(ringbuffer_pop(conn.rxBuffer, &pv_h)) return;
		uint8_t pv_l;
		if(ringbuffer_pop(conn.rxBuffer, &pv_l)) return;
		int16_t pv = (pv_h << 8) | pv_l;

		if(port >= MOTOR_COUNT + 4)
		{
			ringbuffer_push(conn.txBuffer, HCP_INVALID_PORT);
			return;
		}
		if(mode > HCP_MOTOR_MODE_VELOCITY)
		{
			ringbuffer_push(conn.txBuffer, HCP_INVALID_MODE);
			return;
		}
		if((mode == MOTOR_MODE_POWER || mode == MOTOR_MODE_BRAKE))
		{
			if(abs(pv) > MOTOR_MAX_POWER)
			{
				ringbuffer_push(conn.txBuffer, HCP_INVALID_VALUE);
				return;
			}
			if((power_getEmergencyStop()) && (pv != 0))
			{
				ringbuffer_push(conn.txBuffer, HCP_FAIL_EMERG_ACT);
				return;
			}
		}
		else
		{
			if(power_getEmergencyStop())
			{
				ringbuffer_push(conn.txBuffer, HCP_FAIL_EMERG_ACT);
				return;
			}
		}

		motor_set(port, mode, pv);
	}
	else if(opcode == HCP_MOTOR_POSITIONAL) //TODO: add check if motor is of type encoder or stepper, for stepper only velocity and brake modes are allowed
	{
		uint8_t port;
		if(ringbuffer_pop(conn.rxBuffer, &port)) return;

		uint8_t mode;
		if(ringbuffer_pop(conn.rxBuffer, &mode)) return;

		uint8_t pv_h;
		if(ringbuffer_pop(conn.rxBuffer, &pv_h)) return;
		uint8_t pv_l;
		if(ringbuffer_pop(conn.rxBuffer, &pv_l)) return;
		int16_t pv = (pv_h << 8) | pv_l;

		uint8_t rel_doneMode;
		if(ringbuffer_pop(conn.rxBuffer, &rel_doneMode)) return;
		bool relative = rel_doneMode & 0x80;
		uint8_t done_mode = rel_doneMode & ~0x80;

		uint8_t pos_3;
		if(ringbuffer_pop(conn.rxBuffer, &pos_3)) return;
		uint8_t pos_2;
		if(ringbuffer_pop(conn.rxBuffer, &pos_2)) return;
		uint8_t pos_1;
		if(ringbuffer_pop(conn.rxBuffer, &pos_1)) return;
		uint8_t pos_0;
		if(ringbuffer_pop(conn.rxBuffer, &pos_0)) return;
		int32_t pos = (pos_3 << 24) | (pos_2 << 16) | (pos_1 << 8) | pos_0;

		if(port >= MOTOR_COUNT)
		{
			ringbuffer_push(conn.txBuffer, HCP_INVALID_PORT);
			return;
		}
		if(mode > HCP_MOTOR_MODE_VELOCITY)
		{
			ringbuffer_push(conn.txBuffer, HCP_INVALID_MODE);
			return;
		}
		if((mode == MOTOR_MODE_POWER || mode == MOTOR_MODE_BRAKE) && abs(pv) > MOTOR_MAX_POWER)
		{
			ringbuffer_push(conn.txBuffer, HCP_INVALID_VALUE);
			return;
		}
		if(done_mode > HCP_MOTOR_POS_DONE_MODE_ACTIVE_BRAKE)
		{
			ringbuffer_push(conn.txBuffer, HCP_INVALID_MODE);
			return;
		}

		motor_positional(port, mode, pv, done_mode, relative, pos);
	}
	else if(opcode == HCP_MOTOR_SERVO) //TODO: add check if motor is of type encoder
	{
		uint8_t port;
		if(ringbuffer_pop(conn.rxBuffer, &port)) return;

		uint8_t vel_h;
		if(ringbuffer_pop(conn.rxBuffer, &vel_h)) return;
		uint8_t vel_l;
		if(ringbuffer_pop(conn.rxBuffer, &vel_l)) return;
		int16_t vel = (vel_h << 8) | vel_l;

		uint8_t relative;
		if(ringbuffer_pop(conn.rxBuffer, &relative)) return;

		uint8_t pos_3;
		if(ringbuffer_pop(conn.rxBuffer, &pos_3)) return;
		uint8_t pos_2;
		if(ringbuffer_pop(conn.rxBuffer, &pos_2)) return;
		uint8_t pos_1;
		if(ringbuffer_pop(conn.rxBuffer, &pos_1)) return;
		uint8_t pos_0;
		if(ringbuffer_pop(conn.rxBuffer, &pos_0)) return;
		int32_t pos = (pos_3 << 24) | (pos_2 << 16) | (pos_1 << 8) | pos_0;

		if(port >= MOTOR_COUNT)
		{
			ringbuffer_push(conn.txBuffer, HCP_INVALID_PORT);
			return;
		}

		motor_servo(port, vel, relative, pos);
	}
	else if(opcode == HCP_MOTOR_CONFIG_DC)
	{
		uint8_t port;
		if(ringbuffer_pop(conn.rxBuffer, &port)) return;

		if(port >= MOTOR_COUNT)
		{
			ringbuffer_push(conn.txBuffer, HCP_INVALID_PORT);
			return;
		}

		motor_configure(port, MOTOR_TYPE_DC, 0, 0);
	}
	else if(opcode == HCP_MOTOR_CONFIG_ENC)
	{
		uint8_t port;
		if(ringbuffer_pop(conn.rxBuffer, &port)) return;

		uint8_t enc_a;
		if(ringbuffer_pop(conn.rxBuffer, &enc_a)) return;

		uint8_t enc_b;
		if(ringbuffer_pop(conn.rxBuffer, &enc_b)) return;

		if(port >= MOTOR_COUNT || enc_a == enc_b || enc_a > 7 || enc_b > 7)
		{
			ringbuffer_push(conn.txBuffer, HCP_INVALID_PORT);
			return;
		}

		motor_configure(port, MOTOR_TYPE_ENC, enc_a, enc_b);
	}
	else if(opcode == HCP_MOTOR_CONFIG_STEP)
	{
		uint8_t port;
		if(ringbuffer_pop(conn.rxBuffer, &port)) return;

		if(!(port == 0 || port == 2))
		{
			ringbuffer_push(conn.txBuffer, HCP_INVALID_PORT);
			return;
		}

		motor_configure(port, MOTOR_TYPE_STEP, 0, 0);
	}

	ringbuffer_push(conn.txBuffer, HCP_OK);
}
