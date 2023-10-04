#ifndef BASIC_H
#define BASIC_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include "IMQP.h"

/*====================================*/
/* PUBLIC FUNCTIONS */
/*====================================*/

void send_basic_deliver(Connection *connection, char *queue_name,
                        IMQP_Byte *body, uint32_t body_size);

void process_frame_basic(Connection *connection, IMQP_Frame *frame);
void define_pub_body_size(IMQP_Byte *remaining_payload);
void define_pub_body(IMQP_Byte *payload);
void finish_pub();
int get_pub_size();

#endif /* BASIC_H */
