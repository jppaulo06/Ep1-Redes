#ifndef AMQP_H
#define AMQP_H

/*====================================*/
/* DEPENDENCIES HEADERS */
/*====================================*/

#include <stdint.h>

/*====================================*/
/* DEFINED MACROS */
/*====================================*/

#define MAX_PAYLOAD_STORAGE 300000
#define INITIAL_MAX_FRAMES 1000

/*====================================*/
/* PUBLIC ELEMENTS */
/*====================================*/

enum IMQP_Frame_t {
  METHOD_FRAME = 1,
  HEADER_FRAME,
  BODY_FRAME,
  HEARTBEAT_FRAME
};

enum IMQP_Frame_Method_Class {
  CONNECTION_CLASS = 10,
  CHANNEL_CLASS = 20,
  QUEUE_CLASS = 50,
};

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

enum IMQP_Frame_Channel {
  CHANNEL_OPEN = 10,
  CHANNEL_OPEN_OK,
  CHANNEL_CLOSE = 40,
  CHANNEL_CLOSE_OK,
};

enum IMQP_Frame_Queue {
  QUEUE_DECLARE = 10,
  QUEUE_DECLARE_OK,
};

typedef char IMQP_Argument;

typedef struct {
  uint8_t type;
  uint16_t channel;
  uint32_t arguments_size;
  uint16_t class_method;
  uint16_t method;
  IMQP_Argument *arguments;
} IMQP_Frame;

typedef struct {
  uint16_t channel_max;
  uint32_t frame_max;
  uint16_t heartbeat;
} Config;

typedef struct {
  int socket;
  Config config;
} Connection;

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
  IMQP_Queue* list;
} IMQP_Queue_List;

/*====================================*/
/* PUBLIC FUNCTIONS */
/*====================================*/

Connection *new_AMQP_Connection(int socket);
void process_action(Connection *connection);

#endif /* AMQP_H */
