#ifndef HCP_HANDLER_H_
#define HCP_HANDLER_H_


#include <stddef.h>
#include <stdbool.h>
#include "hcp.h"

//handlers
void hcp_handler_version(hcp_conn_t conn, uint8_t opcode, size_t payloadLength);
void hcp_handler_ioConfig(hcp_conn_t conn, uint8_t opcode, size_t payloadLength);
void hcp_handler_analogReq(hcp_conn_t conn, uint8_t opcode, size_t payloadLength);
void hcp_handler_imu(hcp_conn_t conn, uint8_t opcode, size_t payloadLength);
void hcp_handler_digitalReq(hcp_conn_t conn, uint8_t opcode, size_t payloadLength);
void hcp_handler_motor(hcp_conn_t conn, uint8_t opcode, size_t payloadLength);
void hcp_handler_servo(hcp_conn_t conn, uint8_t opcode, size_t payloadLength);
void hcp_handler_uart(hcp_conn_t conn, uint8_t opcode, size_t payloadLength);
void hcp_handler_speaker(hcp_conn_t conn, uint8_t opcode, size_t payloadLength);
void hcp_handler_emergency(hcp_conn_t conn, uint8_t opcode, size_t payloadLength);
void hcp_handler_st(hcp_conn_t conn, uint8_t opcode, size_t payloadLength);


#endif /* HCP_HANDLER_H_ */
