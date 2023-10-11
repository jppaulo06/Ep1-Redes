#ifndef IMQP_H
#define IMQP_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include <stdint.h>
#include <stdio.h>

/*====================================*/
/* PUBLIC MACROS */
/*====================================*/

#define DEBUG_MODE

#define BASE_COMMUNICATION_CHANNEL 0
#define UNIQUE_COMMUNICATION_CHANNEL 1
#define MAX_FRAME_SIZE 3000
#define MAX_QUEUE_NAME_SIZE 1000
#define CONSUMER_TAG_SIZE 50
#define MAX_BODY_SIZE 3000

#define PRESERVE_CONNECTION 0
#define EXIT_CONNECTION 1

/*====================================*/
/* FUNDAMENTALS */
/*====================================*/

typedef char IMQP_Byte;

typedef struct {
  uint16_t channel_max;
  uint32_t frame_max;
} Config;

typedef struct {
  int socket;
  Config config;
  IMQP_Byte consumer_tag[CONSUMER_TAG_SIZE];
  uint8_t consumer_tag_size;
  int is_closed;
  int is_consumer;
} Connection;

typedef struct {
  uint16_t ticket;
  uint8_t name_size;
  char *name;
  int max_connections;
  int total_connections;
  int round_robin;
  Connection **connections;
} IMQP_Queue;

/*====================================*/
/* METHOD ARGUMENTS */
/*====================================*/

typedef struct {
  char queue_name[MAX_QUEUE_NAME_SIZE];
} Basic_Consume;

typedef struct {
  char queue_name[MAX_QUEUE_NAME_SIZE];
} Basic_Publish;

typedef struct {
  uint16_t ticket;
  char queue_name[MAX_QUEUE_NAME_SIZE];
} Queue_Declare;

typedef struct {
  uint16_t channel_max;
  uint32_t frame_max;
} Connection_Tune_Ok;

typedef union {
  Basic_Consume basic_consume; 
  Basic_Publish basic_publish;
  Queue_Declare queue_declare;
  Connection_Tune_Ok connection_tune_ok;
} IMQP_Arguments;

/*====================================*/
/* FRAMES */
/*====================================*/

typedef struct {
  uint16_t class_header;
  uint64_t body_size;
} Head_Payload;

typedef struct {
  uint16_t class_method;
  uint16_t method;
  uint32_t arguments_size;
  IMQP_Arguments arguments;
} Method_Payload;

typedef struct {
  IMQP_Byte body[MAX_BODY_SIZE];
} Body_Payload;

typedef union {
    Head_Payload header_pl;
    Method_Payload method_pl;
    Body_Payload body_pl;
} IMQP_Payload;

typedef struct {
  uint8_t type;
  uint16_t channel;
  uint32_t payload_size;
  IMQP_Payload payload;
} IMQP_Frame;

#endif /* IMQP_H */
