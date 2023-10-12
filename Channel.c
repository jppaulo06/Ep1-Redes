#include "Channel.h"
#include "Frame.h"
#include "super_header.h"

/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

uint64_t process_frame_channel(Connection *connection, Method_Payload payload) {
  uint64_t flags = 0;
  switch ((enum IMQP_Frame_Channel)payload.method) {
  case CHANNEL_OPEN:
    send_channel_open_ok(connection);
    break;
  case CHANNEL_CLOSE:
    send_channel_close_ok(connection);
    break;
  default:
    flags |= NOT_EXPECTED_METHOD;
  }
  return flags;
}
