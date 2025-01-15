#ifndef LOG_H
#define LOG_H

#include <assert.h>
#include <stdio.h>

#define log(format, ...)                                                      \
  printf("\33[1;34m"                                                          \
         "[%s:%d %s] " format "\33[0m"                                        \
         "\n",                                                                \
         __FILE__, __LINE__, __func__, ##__VA_ARGS__);

#define Assert(expr, ...)                                                     \
  if(!(expr))                                                                 \
    {                                                                         \
      log("Assert Failed: %s.", ##__VA_ARGS__);                               \
      assert(expr);                                                           \
    }
#endif /* log.h */