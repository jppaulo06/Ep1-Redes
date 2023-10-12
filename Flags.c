#include "Flags.h"
#include "IMQP.h"
#include <stdio.h>

void print_errors_or_warnings(uint64_t flags) {
  if (flags & ERRORS)
    print_errors(flags);
#if defined(DEBUG_MODE) || defined(SNIFF_MODE)
  if (flags & WARNINGS)
    print_warnings(flags);
#endif /* DEBUG_MODE || SNIFF_MODE */
}

void print_errors(uint64_t flags) {
  for (uint64_t error = FIRST_ERROR; error != LAST_ERROR; error <<= 1) {
    switch ((enum Error)(error & flags)) {
    case NOT_EXPECTED_METHOD:
      fprintf(stderr, "[ERROR] Received a not expected method\n");
      break;
    case NOT_EXPECTED_CLASS:
      fprintf(stderr, "[ERROR] Received a not expected class\n");
      break;
    case INVALID_NAME_SIZE:
      fprintf(stderr, "[ERROR] Received invalid name of queue\n");
      break;
    case INVALID_EXCHANGE_NAME_SIZE:
      fprintf(stderr, "[ERROR] Received invalid name of exchange\n");
      break;
    case INVALID_CONSUMER_TAG_SIZE:
      fprintf(stderr, "[ERROR] Received invalid consumer tag\n");
      break;
    case INVALID_FRAME:
      fprintf(stderr, "[ERROR] Received invalid frame\n");
      break;
    case INVALID_FRAME_TYPE:
      fprintf(stderr, "[ERROR] Received invalid frame type\n");
      break;
    case INVALID_FRAME_CHANNEL:
      fprintf(stderr, "[ERROR] Received invalid frame channel\n");
      break;
    case INVALID_TUNE:
      fprintf(stderr, "[ERROR] Received invalid configs at connection Tune\n");
      break;
    case NO_ERROR:
      break;
    }
  }
}

void print_warnings(uint64_t flags) {
  for (uint64_t warning = FIRST_ERROR; warning != LAST_ERROR; warning <<= 1) {
    switch ((enum Warning)(warning & flags)) {
      case QUEUE_NOT_FOUND_OR_EMPTY:
        fprintf(stderr, "[WARNING] Queue not found or is empty\n");
        break;
      case HEARTBEAT_IGNORED:
        fprintf(stderr, "[WARNING] Heartbeat (not supported) received\n");
        break;
    case NO_WARNING:
      break;
    }
  }
}
