#ifndef QUEUE_H
#define QUEUE_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include "IMQP.h"

/*====================================*/
/* PUBLIC TYPES */
/*====================================*/

enum IMQP_Frame_Queue {
  QUEUE_DECLARE = 10,
  QUEUE_DECLARE_OK,
};

/*====================================*/
/* PUBLIC FUNCTIONS DECLARATIONS */
/*====================================*/

void process_frame_queue(Connection *connection, Method_Payload frame);
void publish_to_queue(char* queue_name, char* body, int body_size);
void put_into_queue(Connection* connection, const char* queue_name);

#endif /* QUEUE_H */
