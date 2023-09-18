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

#endif /* QUEUE_H */
