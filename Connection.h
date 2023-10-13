#ifndef CONNECTION_H
#define CONNECTION_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include <stdint.h>
#include "IMQP.h"

/*====================================*/
/* PUBLIC ENUM */
/*====================================*/

enum IMQP_Frame_Connection {
  CONNECTION_START = 10,
  CONNECTION_START_OK,
  CONNECTION_TUNE = 30,
  CONNECTION_TUNE_OK,
  CONNECTION_OPEN = 40,
  CONNECTION_OPEN_OK,
  CONNECTION_CLOSE = 50,
  CONNECTION_CLOSE_OK,
};

/*====================================*/
/* PUBLIC FUNCTIONS DECLARATIONS */
/*====================================*/

void close_connection(Connection *connection);
void* process_connection(void* fd);
uint64_t process_frame_connection(Connection *connection, Method_Payload payload);

#endif
