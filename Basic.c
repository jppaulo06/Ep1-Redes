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

static uint64_t publish();
static void define_pub_queue_name(char queue_name[MAX_QUEUE_NAME_SIZE]);

static void create_FIN_thread(Connection *connection);
static void *wait_for_FIN(void *fd);

/*====================================*/
/* PRIVATE VARIABLES */
/*====================================*/

struct {
  uint64_t size;
  char queue_name[MAX_QUEUE_NAME_SIZE];
  IMQP_Byte body[MAX_BODY_SIZE];
} publication;

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
    define_pub_queue_name(payload.arguments.basic_publish.queue_name);
    break;
  default:
    flags |= NOT_EXPECTED_METHOD;
  }
  return flags;
}

void define_pub_body_size(uint64_t size) { publication.size = size; }

void define_pub_body(IMQP_Byte *body) {
  memcpy(publication.body, body, publication.size);
}

uint64_t finish_pub() {
#ifdef SNIFF_MODE
  printf("[PUBLISH]\n  Queue name: %s\n  Body: %s\n", publication.queue_name,
         publication.body);
#endif /* SNIFF_MODE */

  return publish();
}

int get_pub_size() { return publication.size; }

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

uint64_t process_basic_consume(Connection *connection,
                               Basic_Consume arguments) {
  uint64_t flags = 0;
  flags |= put_into_queue(connection, arguments.queue_name);
  if (!(flags & ERRORS)) {
    send_basic_consume_ok(connection);
    create_FIN_thread(connection);
    connection->consumer = true;
  }
  return flags;
}

void create_FIN_thread(Connection *connection) {
  pthread_t tid;
  pthread_create(&tid, NULL, wait_for_FIN, connection);
}

void *wait_for_FIN(void *conn) {
  Connection *connection = (Connection *)conn;
  uint64_t socketfd = connection->socket;
  IMQP_Byte buffer[MAX_FRAME_SIZE];
  int ret = read(socketfd, buffer, MAX_FRAME_SIZE);
  if (ret > 0) {
    return wait_for_FIN(connection);
  }
  close_connection(connection);
  return NULL;
}

void define_pub_queue_name(char queue_name[MAX_QUEUE_NAME_SIZE]) {
  strcpy(publication.queue_name, queue_name);
}

uint64_t publish() {
  return publish_to_queue(publication.queue_name, publication.body,
                          publication.size);
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
