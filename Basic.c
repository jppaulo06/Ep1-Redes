#include "Basic.h"
#include "Frame.h"
#include "Queue.h"
#include "super_header.h"
#include <stdio.h>
#include <string.h>

/*====================================*/
/* PRIVATE ENUMS */
/*====================================*/

enum IMQP_Frame_Basic {
  BASIC_CONSUME = 20,
  BASIC_CONSUME_OK,
};

/*====================================*/
/* PRIVATE FUNCTIONS */
/*====================================*/

static void create_new_consumer(Connection *connection,
                                IMQP_Argument *arguments);
static void send_basic_consume_ok(Connection *connection);
static char *create_consumer_tag(uint8_t *size);

/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

void process_frame_basic(Connection *connection, IMQP_Frame *frame) {
  IMQP_Argument *arguments = frame->arguments;
  switch ((enum IMQP_Frame_Basic)frame->method) {
  case BASIC_CONSUME:
    create_new_consumer(connection, arguments);
    send_basic_consume_ok(connection);
    /* Start thread here!!!! */
    break;
  case BASIC_CONSUME_OK:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  default:
    THROW("Not suported");
  }
}

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

void create_new_consumer(Connection *connection, IMQP_Argument *arguments) {
  void *index = arguments;

  uint16_t garbage;
  uint8_t name_size;

  message_break_s(&garbage, &index);
  message_break_b(&name_size, &index);

  char *queue_name = Malloc(name_size + 1);

  message_break_n(queue_name, &index, name_size);
  queue_name[name_size] = '\x00';

  put_into_queue(connection, queue_name);
}

void send_basic_consume_ok(Connection *connection) {

  const char *consumer_tag = "amq.ctag-5jjMjY64MJ481wNlmu9UJg";
  uint8_t consumer_tag_size = strlen(consumer_tag);

  uint8_t type = METHOD_FRAME;

  uint16_t channel = UNIQUE_COMMUNICATION_CHANNEL;

  uint16_t class = BASIC_CLASS;
  uint16_t method = BASIC_CONSUME_OK;

  uint32_t payload_size = sizeof(class) + sizeof(method) + consumer_tag_size;

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
  message_build_b(&index, &consumer_tag_size);
  message_build_n(&index, (void*) consumer_tag, consumer_tag_size);
  message_build_b(&index, FRAME_END);

  Write(connection->socket, (const char *)message, index - (void *)message);
}

char *create_consumer_tag(uint8_t *size) {
  const char *ctag_default = "amq.ctag-";
  const int ctag_default_len = strlen(ctag_default);

  *size = *size + ctag_default_len;
  char *consumer_tag = Malloc(*size);
  strcpy(consumer_tag, ctag_default);
  printf("consumer tag: %s\n", consumer_tag);
  for (int i = ctag_default_len; i < *size; i++) {
    consumer_tag[i] = (rand() % ('Z' - 'A')) + 'A';
  }
  return consumer_tag;
}
