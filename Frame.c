#include <arpa/inet.h>
#include <string.h>

#include <stdio.h>
#include <unistd.h>

#include "Basic.h"
#include "Channel.h"
#include "Connection.h"
#include "Frame.h"
#include "Queue.h"
#include "super_header.h"

/*====================================*/
/* PRIVATE MACROS */
/*====================================*/

#define INITIAL_MAX_FRAMES 1000

/* From
 * https://stackoverflow.com/questions/3022552/is-there-any-standard-htonl-like-function-for-64-bits-integers-in-c
 */
#if __BIG_ENDIAN__
#define htonll(x) (x)
#define ntohll(x) (x)
#else
#define htonll(x) (((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) (((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

static IMQP_Frame *new_IMQP_Frame();
static void throw_frame_away(Connection *connection);
static void receive_frame(Connection *connection, IMQP_Frame *frame);

static void receive_frame_header(Connection *connection, IMQP_Frame *frame);
static void receive_frame_payload(Connection *connection, IMQP_Frame *frame);
static void receive_frame_end(Connection *connection);

static void release_frame(IMQP_Frame **frame);

static void process_method_frame(Connection *connection, IMQP_Frame *frame);
static void process_header_frame(Connection *connection, IMQP_Frame *frame);
static void process_body_frame(Connection *connection, IMQP_Frame *frame);

/*====================================*/
/* PUBLIC FUNCTIONS DECLARATIONS */
/*====================================*/

void process_frame(Connection *connection) {
  if (connection->is_closed || connection->is_consumer) {
    return;
  }
  IMQP_Frame *frame = new_IMQP_Frame();
  receive_frame(connection, frame);
  switch ((enum IMQP_Frame_t)frame->type) {
  case METHOD_FRAME:
    process_method_frame(connection, frame);
    break;
  case HEADER_FRAME:
    process_header_frame(connection, frame);
    break;
  case BODY_FRAME:
    process_body_frame(connection, frame);
    break;
  case HEARTBEAT_FRAME:
    THROW("Not implemented frame");
    break;
  }
  release_frame(&frame);
  process_frame(connection);
}

/* Deserialization functions */

void message_break_b(uint8_t *content, void **index) {
  memcpy(content, *index, sizeof(*content));
  *index += sizeof(*content);
}

void message_break_s(uint16_t *content, void **index) {
  memcpy(content, *index, sizeof(*content));
  *content = ntohs(*content);
  *index += sizeof(*content);
}

void message_break_l(uint32_t *content, void **index) {
  memcpy(content, *index, sizeof(*content));
  *content = ntohl(*content);
  *index += sizeof(*content);
}

void message_break_ll(uint64_t *content, void **index) {
  memcpy(content, *index, sizeof(*content));
  *content = ntohll(*content);
  *index += sizeof(*content);
}

void message_break_n(void *content, void **index, uint16_t n) {
  memcpy(content, *index, n);
  *index += n;
}

/* Serialization functions */

void message_build_b(void **index, uint8_t *content) {
  memcpy(*index, content, sizeof(*content));
  *index += sizeof(*content);
}

void message_build_s(void **index, uint16_t *content) {
  *content = htons(*content);
  memcpy(*index, content, sizeof(*content));
  *index += sizeof(*content);
}

void message_build_l(void **index, uint32_t *content) {
  *content = htonl(*content);
  memcpy(*index, content, sizeof(*content));
  *index += sizeof(*content);
}

void message_build_ll(void **index, uint64_t *content) {
  *content = htonll(*content);
  memcpy(*index, content, sizeof(*content));
  *index += sizeof(*content);
}

void message_build_n(void **index, void *content, uint16_t n) {
  memcpy(*index, content, n);
  *index += n;
}

int send_heartbeat(Connection *connection) {
  uint8_t type = HEARTBEAT_FRAME;
  uint16_t channel = BASE_COMMUNICATION_CHANNEL;

  uint32_t payload_size = 0;

  /* + 1 because of frame-end %xCE */
  uint32_t message_size =
      sizeof(type) + sizeof(channel) + sizeof(payload_size) + payload_size + 1;

  uint8_t *message = Malloc(message_size);

  void *index = message;

  message_build_b(&index, &type);
  message_build_s(&index, &channel);
  message_build_l(&index, &payload_size);
  message_build_b(&index, FRAME_END);

  int ret =
      write(connection->socket, (const char *)message, index - (void *)message);

  if(ret <= 0) {
    printf("Morri! connection: %p, return: %d\n", connection->consumer_tag, ret);
  }

  free(message);

  return ret <= 0;
}

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

IMQP_Frame *new_IMQP_Frame() {
  IMQP_Frame *frame = Malloc(sizeof(*frame));
  bzero(frame, sizeof(*frame));
  return frame;
}

void throw_frame_away(Connection *connection) {
  IMQP_Frame *frame = new_IMQP_Frame();
  receive_frame(connection, frame);
  release_frame(&frame);
}

void receive_frame(Connection *connection, IMQP_Frame *frame) {
  receive_frame_header(connection, frame);
  receive_frame_payload(connection, frame);
  receive_frame_end(connection);
}

void receive_frame_header(Connection *connection, IMQP_Frame *frame) {

  IMQP_Byte buffer[20] = {0};

  Read(connection->socket, buffer, 7);

  void *offset = buffer;

  message_break_b(&frame->type, &offset);
  message_break_s(&frame->channel, &offset);
  message_break_l(&frame->payload_size, &offset);
}

void receive_frame_payload(Connection *connection, IMQP_Frame *frame) {
  IMQP_Byte buffer[frame->payload_size];
  uint16_t garbage;
  Read(connection->socket, buffer, frame->payload_size);
  void *offset = buffer;
  switch (frame->type) {
  case METHOD_FRAME:
    message_break_s(&frame->payload.method_pl.class_method, &offset);
    message_break_s(&frame->payload.method_pl.method, &offset);

    frame->payload.method_pl.arguments_size =
        frame->payload_size - (offset - (void *)buffer);

    if (frame->payload.method_pl.arguments_size > 0) {

      frame->payload.method_pl.arguments =
          Malloc(frame->payload.method_pl.arguments_size);

      message_break_n(frame->payload.method_pl.arguments, &offset,
                      frame->payload.method_pl.arguments_size);
    }
    break;
  case HEADER_FRAME:
    message_break_s(&frame->payload.header_pl.class_header, &offset);

    frame->payload.header_pl.remaining_payload_size =
        frame->payload_size - (offset - (void *)buffer);

    frame->payload.header_pl.remaining_payload =
        Malloc(frame->payload.header_pl.remaining_payload_size);

    message_break_n(frame->payload.header_pl.remaining_payload, &offset,
                    frame->payload.header_pl.remaining_payload_size);
    break;
  case BODY_FRAME:
    frame->payload.body_pl.body = Malloc(get_pub_size());
    message_break_n(frame->payload.body_pl.body, &offset, get_pub_size());
    break;
  case HEARTBEAT_FRAME:
    THROW("Not implemented frame");
    break;
  }
}

void receive_frame_end(Connection *connection) {
  IMQP_Byte garbage = 0;
  Read(connection->socket, &garbage, 1);
}

void release_frame(IMQP_Frame **frame) {
  switch ((*frame)->type) {
  case METHOD_FRAME:
    free((*frame)->payload.method_pl.arguments);
    break;

  case HEADER_FRAME:
    free((*frame)->payload.header_pl.remaining_payload);
    break;

  case BODY_FRAME:
    free((*frame)->payload.body_pl.body);
    break;

  case HEARTBEAT_FRAME:
    THROW("Not implemented method");
    break;
  }

  free(*frame);
  *frame = NULL;
}

void process_method_frame(Connection *connection, IMQP_Frame *frame) {
  switch ((enum IMQP_Frame_Class)frame->payload.method_pl.class_method) {
  case QUEUE_CLASS:
    process_frame_queue(connection, frame);
    break;
  case CONNECTION_CLASS:
    process_frame_connection(connection, frame);
    break;
  case CHANNEL_CLASS:
    process_frame_channel(connection, frame);
    break;
  case BASIC_CLASS:
    process_frame_basic(connection, frame);
    break;
  default:
    THROW("Not implemented class");
  }
}

void process_header_frame(Connection *connection, IMQP_Frame *frame) {
  switch ((enum IMQP_Frame_Class)frame->payload.header_pl.class_header) {
  case BASIC_CLASS:
    define_pub_body_size(frame->payload.header_pl.remaining_payload);
    break;
  default:
    THROW("Not implemented class");
  }
}

void process_body_frame(Connection *connection, IMQP_Frame *frame) {
  printf("Estou aqui tentando fazer um publish, mas algo da errado :/\n");
  define_pub_body(frame->payload.body_pl.body);
  finish_pub();
}
