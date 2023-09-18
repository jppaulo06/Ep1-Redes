#ifndef CONNECTION_H
#define CONNECTION_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include <stdint.h>
#include "IMQP.h"

/*====================================*/
/* PUBLIC FUNCTIONS DECLARATIONS */
/*====================================*/

void process_connection(int socket);
void process_frame_connection(Connection *connection, IMQP_Frame *frame);
int connection_is_off(Connection *connection);

#endif
