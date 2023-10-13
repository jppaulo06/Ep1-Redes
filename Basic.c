#include "Basic.h"
#include "Connection.h"
#include "Frame.h"
#include "Queue.h"
#include "super_header.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <stdio.h>

/*====================================*/
/* GLOBAL FUNCTIONS */
/*====================================*/

void define_consumer_tag(Connection *connection);

/*====================================*/
/* PRIVATE FUNCTIONS */
/*====================================*/

static uint64_t process_basic_consume(Connection *connection,
                                      Basic_Consume arguments);
static uint64_t process_basic_publish(Connection *connection,
                                      char queue_name[MAX_QUEUE_NAME_SIZE]);

/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

uint64_t process_frame_basic(Connection *connection, Method_Payload payload) {
  uint64_t flags = 0;
  switch ((enum IMQP_Frame_Basic)payload.method) {
  case BASIC_CONSUME:
    flags |= process_basic_consume(connection, payload.arguments.basic_consume);
    break;
  case BASIC_PUBLISH:
    flags |= process_basic_publish(connection,
                                   payload.arguments.basic_publish.queue_name);
    break;
  case BASIC_ACK:
    break;
  default:
    flags |= NOT_EXPECTED_METHOD;
  }
  return flags;
}

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

uint64_t process_basic_consume(Connection *connection,
                               Basic_Consume arguments) {
  uint64_t flags = 0;
  flags |= put_into_queue(connection, arguments.queue_name);
  if (!(flags & ERRORS)) {
    send_basic_consume_ok(connection);
    connection->consumer = true;
  }
  return flags;
}

uint64_t process_basic_publish(Connection *connection,
                               char queue_name[MAX_QUEUE_NAME_SIZE]) {
  strcpy(connection->publication.queue_name, queue_name);
  return NO_ERROR;
}

void define_consumer_tag(Connection *connection) {
  const char *ctag_default = "amq.ctag-";
  const uint8_t ctag_default_size = 9;

  connection->consumer_tag_size = CONSUMER_TAG_SIZE;

  memcpy(connection->consumer_tag, ctag_default, ctag_default_size);

  for (uint8_t i = ctag_default_size; i < connection->consumer_tag_size; i++) {
    connection->consumer_tag[i] = (uint8_t)((rand() % ('Z' - 'A')) + 'A');
  }
}
