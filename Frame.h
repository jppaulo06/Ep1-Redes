#ifndef FRAME_H
#define FRAME_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include "stdint.h"
#include "IMQP.h"

/*====================================*/
/* PRIVATE MACROS */
/*====================================*/

#define FRAME_END ((uint8_t)'\xCE')

/*====================================*/
/* PUBLIC ENUMS */
/*====================================*/

enum IMQP_Frame_t {
  METHOD_FRAME = 1,
  HEADER_FRAME,
  BODY_FRAME,
  HEARTBEAT_FRAME = 8,
  MAX_FRAME_TYPE
};

enum IMQP_Frame_Class {
  CONNECTION_CLASS = 10,
  CHANNEL_CLASS = 20,
  QUEUE_CLASS = 50,
  BASIC_CLASS = 60,
};

/*====================================*/
/* PUBLIC FUNCTIONS */
/*====================================*/

uint64_t receive_protocol_header(Connection *connection);
uint64_t receive_and_process_frame(Connection *connection);

void send_protocol_header(Connection* connection);

void send_channel_open_ok(Connection *connection);
void send_channel_close_ok(Connection *connection);
void send_queue_declare_ok(Connection *connection, IMQP_Queue *queue);
void send_queue_declare_ok(Connection *connection, IMQP_Queue *queue);
void send_basic_consume_ok(Connection *connection);

void send_connection_start(Connection *connection);
void send_connection_tune(Connection *connection);
void send_connection_open_ok(Connection *connection);
void send_connection_start(Connection *connection);
void send_connection_close_ok(Connection *connection);

#endif /* FRAME_H */
