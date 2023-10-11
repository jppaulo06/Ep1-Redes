#include "Channel.h"
#include "Frame.h"
#include "super_header.h"

/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

void process_frame_channel(Connection *connection, Method_Payload payload) {
  switch ((enum IMQP_Frame_Channel)payload.method) {
  case CHANNEL_OPEN:
    send_channel_open_ok(connection);
    break;
  case CHANNEL_OPEN_OK:
    fprintf(stderr, "[WARNING] Received a not expected channel open ok\n");
    break;
  case CHANNEL_CLOSE:
    send_channel_close_ok(connection);
    break;
  case CHANNEL_CLOSE_OK:
    fprintf(stderr, "[WARNING] Received a not expected channel close ok\n");
    break;
  default:
    fprintf(stderr, "[WARNING] Received a not expected frame method\n");
  }
}

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

