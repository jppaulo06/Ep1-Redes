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
Connection* get_connection_from_queue(const char *queue_name);
void put_into_queue(Connection* connection, const char* queue_name);

#endif /* QUEUE_H */
