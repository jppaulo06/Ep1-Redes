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

uint64_t process_frame_queue(Connection *connection, Method_Payload payload);
uint64_t publish(Connection* connection);
uint64_t put_into_queue(Connection* connection, const char* queue_name);

#endif /* QUEUE_H */
