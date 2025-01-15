#ifndef COMMONS_H
#define COMMONS_H

#include <stdint.h>
#define i64 int64_t
#define u8 unsigned char

i64 getFileSize(char* path);
i64 getLastAccessInterval(char* filename);

#endif /* commons.h */