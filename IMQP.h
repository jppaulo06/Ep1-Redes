#ifndef IMQP_H
#define IMQP_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include <stdint.h>

/*====================================*/
/* PUBLIC MACROS */
/*====================================*/

#define BASE_COMMUNICATION_CHANNEL 0
#define UNIQUE_COMMUNICATION_CHANNEL 1
#define MAX_FRAME_SIZE 3000

#define PRESERVE_CONNECTION 0
#define EXIT_CONNECTION 1

/*====================================*/
/* FUNDAMENTALS FRAME STRUCTURES */
/*====================================*/

typedef char IMQP_Byte;

typedef struct {
  uint16_t class_header;
  uint64_t body_size;
  uint32_t remaining_payload_size;
  IMQP_Byte* remaining_payload;
} Head_Payload;

typedef struct {
  IMQP_Byte *body;
} Body_Payload;

typedef struct {
  uint16_t class_method;
  uint16_t method;
  uint32_t arguments_size;
  IMQP_Byte *arguments;
} Method_Payload;

union IMQP_Payload {
    Method_Payload method_pl;
    Head_Payload header_pl;
    Body_Payload body_pl;
};

typedef struct {
  uint8_t type;
  uint16_t channel;
  uint32_t payload_size;
  union IMQP_Payload payload;
} IMQP_Frame;

/*====================================*/
/* FUNDAMENTALS CONNECTION STRUCTURES */
/*====================================*/

typedef struct {
  uint16_t channel_max;
  uint32_t frame_max;
  uint16_t heartbeat;
} Config;

typedef struct {
  int socket;
  Config config;
  IMQP_Byte* consumer_tag;
  uint8_t consumer_tag_size;
  int is_closed;
  int is_consumer;
} Connection;

#endif /* IMQP_H */
