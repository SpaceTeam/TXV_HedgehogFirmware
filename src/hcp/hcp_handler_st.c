#include "hcp_handler.h"
#include "hcp_opcodes.h"
#include "ringbuffer.h"
#include "st.h"

void hcp_handler_st(hcp_conn_t conn, uint8_t opcode, size_t payloadLength)
{
	if(opcode == HCP_ST_THRUST_REQ)
	{
		int32_t data[3] = {0, 0, 0};
		data[0] = st_getThrust(0);
		data[1] = st_getThrust(1);
		data[2] = st_getThrust(2);

		ringbuffer_push(conn.txBuffer, HCP_ST_THRUST_REP);
		ringbuffer_push(conn.txBuffer, (uint8_t)(data[0] >> 16));
		ringbuffer_push(conn.txBuffer, (uint8_t)(data[0] >> 8));
		ringbuffer_push(conn.txBuffer, (uint8_t)(data[0] >> 0));
		ringbuffer_push(conn.txBuffer, (uint8_t)(data[1] >> 16));
		ringbuffer_push(conn.txBuffer, (uint8_t)(data[1] >> 8));
		ringbuffer_push(conn.txBuffer, (uint8_t)(data[1] >> 0));
		ringbuffer_push(conn.txBuffer, (uint8_t)(data[2] >> 16));
		ringbuffer_push(conn.txBuffer, (uint8_t)(data[2] >> 8));
		ringbuffer_push(conn.txBuffer, (uint8_t)(data[2] >> 0));
	}
	//TODO: else error handling
}
