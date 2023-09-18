#include "Queue.h"
#include "Frame.h"
#include "super_header.h"

/*====================================*/
/* PRIVATE MACROS */
/*====================================*/

#define INITIAL_MAX_CONNECTIONS 1000

/*====================================*/
/* PRIVATE ENUMS */
/*====================================*/

enum IMQP_Frame_Queue {
  QUEUE_DECLARE = 10,
  QUEUE_DECLARE_OK,
};

/*====================================*/
/* PRIVATE STRUCTURES */
/*====================================*/

typedef struct {
  uint16_t ticket;
  uint8_t name_size;
  char *name;
  int max_connections;
  int total_connections;
  int round_robin;
  Connection *connections;
} IMQP_Queue;

typedef struct {
  int total_connections;
  IMQP_Queue *list;
} IMQP_Queue_List;

/*====================================*/
/* PRIVATE FUNCTIONS DECLARATIONS */
/*====================================*/

static IMQP_Queue *new_IMQP_Queue(IMQP_Argument *argument);
static void send_queue_declare_ok(Connection *connection, IMQP_Queue *queue);

/*====================================*/
/* PUBLIC FUNCTIONS DEFINITIONS */
/*====================================*/

void process_frame_queue(Connection *connection, IMQP_Frame *frame) {
  IMQP_Argument *arguments = frame->arguments;
  IMQP_Queue *queue = NULL;
  switch ((enum IMQP_Frame_Queue)frame->method) {
  case QUEUE_DECLARE:
    queue = new_IMQP_Queue(arguments);
    // add_to_queue_list(queue);
    send_queue_declare_ok(connection, queue);
    break;
  case QUEUE_DECLARE_OK:
    THROW("Wtf is happening? Am I talking to a server?");
    break;
  default:
    THROW("Not suported");
  }
}

/*====================================*/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*====================================*/

IMQP_Queue *new_IMQP_Queue(IMQP_Argument *arguments) {
  IMQP_Queue *queue = Malloc(sizeof(*queue));

  void *index = arguments;

  message_break_s(&queue->ticket, &index);
  message_break_b(&queue->name_size, &index);

  queue->name = Malloc(queue->name_size + 1);

  message_break_n(queue->name, &index, queue->name_size);
  queue->name[queue->name_size] = '\x00';

  queue->connections = Malloc(sizeof(Connection) * INITIAL_MAX_CONNECTIONS);

  queue->max_connections = INITIAL_MAX_CONNECTIONS;
  queue->total_connections = 0;
  queue->round_robin = 0;

  return queue;
}

void send_queue_declare_ok(Connection *connection, IMQP_Queue *queue) {

  uint8_t type = METHOD_FRAME;
  uint16_t channel = UNIQUE_COMMUNICATION_CHANNEL;

  uint16_t class = QUEUE_CLASS;
  uint16_t method = QUEUE_DECLARE_OK;
  uint32_t message_count = 0;
  uint32_t consumer_count = 0;

  /* + 1 because of name size */
  uint32_t payload_size = sizeof(class) + sizeof(method) +
                          (1 + queue->name_size) + sizeof(message_count) +
                          sizeof(consumer_count);

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
  message_build_b(&index, &queue->name_size);
  message_build_n(&index, queue->name, queue->name_size);
  message_build_l(&index, &message_count);
  message_build_l(&index, &consumer_count);
  message_build_b(&index, FRAME_END);

  Write(connection->socket, (const char *)message, index - (void *)message);
}
