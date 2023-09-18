#ifndef SUPER_HEADER_H
#define SUPER_HEADER_H

/*====================================*/
/* DEPENDENCIES */
/*====================================*/

#include <stdlib.h>

/*====================================*/
/* SUPER FUNCTIONS */
/*====================================*/

void Throw(const char *CUSTOM_ERROR_STRING, const char *file,
                  const char *function, int line);

ssize_t Read_func(int fildes, const char *buf, size_t nbyte,
                         const char *file, const char *function, int line);

ssize_t Write_func(int fildes, const char *buf, size_t nbyte,
                          const char *file, const char *function, int line);

void *Malloc_func(size_t size, const char *file, const char *function,
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

#endif /* SUPER_HEADER_H */
