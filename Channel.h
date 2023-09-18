#ifndef CHANNEL_H
#define CHANNEL_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include "IMQP.h"

/*====================================*/
/* PUBLIC FUNCTIONS DECLARATIONS */
/*====================================*/

void process_frame_channel(Connection *connection, IMQP_Frame *frame);

#endif /* CHANNEL_H */
