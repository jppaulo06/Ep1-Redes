#ifndef SUPER_MACROS_H
#define SUPER_MACROS_H

#include <stdlib.h>

extern void Throw(const char *CUSTOM_ERROR_STRING, const char *file,
                  const char *function, int line);

extern ssize_t Read_func(int fildes, const char *buf, size_t nbyte,
                         const char *file, const char *function, int line);

extern ssize_t Write_func(int fildes, const char *buf, size_t nbyte,
                          const char *file, const char *function, int line);

extern void *Malloc_func(size_t size, const char *file, const char *function,
                         int line);

#define THROW(CUSTOM_ERROR_STRING)                                             \
  do {                                                                         \
    Throw(CUSTOM_ERROR_STRING, __FILE__, __FUNCTION__, __LINE__);              \
  } while (0)

#define Read(fildes, buf, nbyte)                                               \
  Read_func(fildes, buf, nbyte, __FILE__, __FUNCTION__, __LINE__)

#define Write(fildes, buf, nbyte)                                              \
  Write_func(fildes, buf, nbyte, __FILE__, __FUNCTION__, __LINE__)

#define Malloc(size) Malloc_func(size, __FILE__, __FUNCTION__, __LINE__)

#endif
