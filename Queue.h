#ifndef QUEUE_H
#define QUEUE_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include "IMQP.h"

/*====================================*/
/* PUBLIC FUNCTIONS DECLARATIONS */
/*====================================*/

void process_frame_queue(Connection *connection, IMQP_Frame *frame);
void publish_to_queue(char* queue_name, char* body, int body_size);
void put_into_queue(Connection* connection, const char* queue_name);

#endif /* QUEUE_H */
