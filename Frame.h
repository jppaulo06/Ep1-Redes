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

#define FRAME_END ((uint8_t *)"\xCE")

/*====================================*/
/* PUBLIC ENUMS */
/*====================================*/

enum IMQP_Frame_t {
  METHOD_FRAME = 1,
  HEADER_FRAME,
  BODY_FRAME,
  HEARTBEAT_FRAME = 8,
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

void process_frame(Connection *connection);

/* serialization functions */

void message_break_b(uint8_t *content, void **index);
void message_break_s(uint16_t *content, void **index);
void message_break_l(uint32_t *content, void **index);
void message_break_ll(uint64_t *content, void **index);
void message_break_n(void *content, void **index, uint16_t n);

/* deserialization functions */

void message_build_b(void **index, uint8_t *content);
void message_build_s(void **index, uint16_t *content);
void message_build_l(void **index, uint32_t *content);
void message_build_ll(void **index, uint64_t *content);
void message_build_n(void **index, void *content, uint16_t n);

int send_heartbeat(Connection* connection);

#endif /* FRAME_H */
