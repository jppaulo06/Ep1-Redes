#ifndef BASIC_H
#define BASIC_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include "IMQP.h"

/*====================================*/
/* PUBLIC ENUMS */
/*====================================*/

enum IMQP_Frame_Basic {
  BASIC_CONSUME = 20,
  BASIC_CONSUME_OK,
  BASIC_PUBLISH = 40,
  BASIC_DELIVER = 60,
  BASIC_ACK = 80,
};

/*====================================*/
/* PUBLIC FUNCTIONS */
/*====================================*/

void send_basic_deliver(Connection *connection, char *queue_name,
                        IMQP_Byte *body, uint32_t body_size);

uint64_t process_frame_basic(Connection *connection, Method_Payload payload);

#endif /* BASIC_H */
