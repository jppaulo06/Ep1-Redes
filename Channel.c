#include "Channel.h"
#include "Frame.h"
#include "super_header.h"

/*====================================*/
/* PRIVATE ENUMS */
/*====================================*/

enum IMQP_Frame_Channel {
  CHANNEL_OPEN = 10,
  CHANNEL_OPEN_OK,
  CHANNEL_CLOSE = 40,
  CHANNEL_CLOSE_OK,
};

/*====================================*/
/* EXTERNAL VARIABLES */
/*====================================*/

extern const char *FAKE_CHANNEL_OPEN_OK;
extern const int FAKE_CHANNEL_OPEN_OK_SIZE;

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

static void send_channel_open_ok(Connection *connection);
static void send_channel_close_ok(Connection *connection);

/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

void process_frame_channel(Connection *connection, IMQP_Frame *frame) {
  switch ((enum IMQP_Frame_Channel)frame->payload.method_pl.method) {
  case CHANNEL_OPEN:
    send_channel_open_ok(connection);
    break;
  case CHANNEL_OPEN_OK:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  case CHANNEL_CLOSE:
    send_channel_close_ok(connection);
    break;
  case CHANNEL_CLOSE_OK:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  default:
    THROW("Not suported");
  }
}

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

void send_channel_open_ok(Connection *connection) {
  Write(connection->socket, FAKE_CHANNEL_OPEN_OK, FAKE_CHANNEL_OPEN_OK_SIZE);
}

void send_channel_close_ok(Connection *connection) {
  uint8_t type = METHOD_FRAME;
  uint16_t channel = UNIQUE_COMMUNICATION_CHANNEL;

  uint16_t class = CHANNEL_CLASS;
  uint16_t method = CHANNEL_CLOSE_OK;

  uint32_t payload_size = sizeof(class) + sizeof(method);

  /* + 1 because of frame-end %xCE */
  uint32_t message_size =
      sizeof(type) + sizeof(channel) + sizeof(payload_size) + payload_size + 1;

  uint8_t *message = Malloc(message_size);

  void *index = message;

  message_build_b(&index, &type);
  message_build_s(&index, &channel);
  message_build_l(&index, &payload_size);
  message_build_s(&index, &class);
  message_build_s(&index, &method);
  message_build_b(&index, FRAME_END);

  Write(connection->socket, (const char *)message, index - (void *)message);
}
