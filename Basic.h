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
};

/*====================================*/
/* PUBLIC FUNCTIONS */
/*====================================*/

void send_basic_deliver(Connection *connection, char *queue_name,
                        IMQP_Byte *body, uint32_t body_size);

void process_frame_basic(Connection *connection, Method_Payload payload);
void define_pub_body_size(uint64_t size);
void define_pub_body(IMQP_Byte *body);
void finish_pub();
int get_pub_size();

#endif /* BASIC_H */
