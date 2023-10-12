#ifndef CHANNEL_H
#define CHANNEL_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include "IMQP.h"

/*====================================*/
/* PUBLIC ENUM */
/*====================================*/

enum IMQP_Frame_Channel {
  CHANNEL_OPEN = 10,
  CHANNEL_OPEN_OK,
  CHANNEL_CLOSE = 40,
  CHANNEL_CLOSE_OK,
};

/*====================================*/
/* PUBLIC FUNCTIONS DECLARATIONS */
/*====================================*/

uint64_t process_frame_channel(Connection *connection, Method_Payload payload);

#endif /* CHANNEL_H */
