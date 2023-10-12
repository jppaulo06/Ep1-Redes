#ifndef IMQP_H
#define IMQP_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include "Flags.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/*====================================*/
/* PUBLIC MACROS */
/*====================================*/

// #define DEBUG_MODE
#define SNIFF_MODE

#define BASE_COMMUNICATION_CHANNEL 0
#define UNIQUE_COMMUNICATION_CHANNEL 1
#define MAX_FRAME_SIZE 3000
#define MAX_QUEUE_NAME_SIZE 1000
#define MAX_EXCHANGE_NAME_SIZE 50
#define CONSUMER_TAG_SIZE 50
#define MAX_BODY_SIZE 3000
#define MAX_STRING_PROP_SIZE 50

#define MAJOR_VERSION 0
#define MINOR_VERSION 9

#define PRESERVE_CONNECTION 0
#define EXIT_CONNECTION 1

/*====================================*/
/* FUNDAMENTALS */
/*====================================*/

typedef char IMQP_Byte;

typedef struct {
  uint8_t major_version;
  uint8_t minor_version;
  char cluster_name[MAX_STRING_PROP_SIZE];
  char license[MAX_STRING_PROP_SIZE];
  char product[MAX_STRING_PROP_SIZE];
  char mechanisms[MAX_STRING_PROP_SIZE];
  char locales[MAX_STRING_PROP_SIZE];
} Meta;

typedef struct {
  uint16_t channel_max;
  uint32_t frame_max;
  uint16_t heartbeat;
} Config;

// TODO: Make the Son of a greve be instanced and put the configs there (like this)
typedef struct {
  const Meta meta;
  const Config config;
} SOAG;

typedef struct {
  int socket;
  Meta meta;
  Config config;
  IMQP_Byte consumer_tag[CONSUMER_TAG_SIZE];
  uint8_t consumer_tag_size;
  bool consumer;
  bool closed;
  bool started;
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
  uint16_t ticket;
  char queue_name[MAX_QUEUE_NAME_SIZE];
  char consumer_tag[CONSUMER_TAG_SIZE];
  uint8_t config_flags;
} Basic_Consume;

typedef struct {
  uint16_t ticket;
  uint8_t exchange;
  char exchange_name[MAX_EXCHANGE_NAME_SIZE];
  char queue_name[MAX_QUEUE_NAME_SIZE];
  uint8_t config_flags;
} Basic_Publish;

typedef struct {
  uint16_t ticket;
  char queue_name[MAX_QUEUE_NAME_SIZE];
} Queue_Declare;

typedef struct {
  uint16_t channel_max;
  uint32_t frame_max;
  uint16_t heartbeat;
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
  uint16_t weight;
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
