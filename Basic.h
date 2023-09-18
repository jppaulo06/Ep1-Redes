#ifndef BASIC_H
#define BASIC_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include "IMQP.h"

/*====================================*/
/* PUBLIC FUNCTIONS */
/*====================================*/

void process_frame_basic(Connection *connection, IMQP_Frame *frame);

#endif /* BASIC_H */

