#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>

enum target
{
  MEMORY = 0,
  _FILE
};

enum req_type
{
  GET = 0,
  POST
};

typedef size_t (*_handle_t)(const void* data_ptr, size_t size, size_t nmemb,
                            void* userdata);

typedef struct response
{
  size_t status_code;
  size_t size;
  char* response_body;
} res_t;

typedef struct __post
{
  char** headers;
  int headerNum;
  char* preCompiledURL;
  char* preCompiledData;
  // Only effects while targetVolume is _FILE.
  char* filename;
  unsigned targetVolume;
} post_t;

typedef struct request
{
  int request_type;
  char* URL;
  // Default NULL, no User-Agent: header is used.
  char* user_agent;
  // Headers list
  char** headers;
  // Headers Num
  int headerNum;
  // Post Data
  char* body;
  // Only effects while targetVolume is FILE.
  char* filename;
  // Actually bool, 0=use ssl and 1=disable.
  int use_ssl;
  // If not set, will use 3 sec as default.
  int timeout;
  // Use memory as default.
  int targetVolume;
  // 1 for yes, 0 for no.
  int followLocation;
  // Use callback function needed.
  _handle_t cb;
} req_t;

typedef struct __get
{
  size_t bid;
  size_t uid;
  size_t sid;
  size_t legacy_only;
  unsigned mods;
  unsigned mode;
  char* _mode;
  char* checksum;
} get_t;

size_t mem_cb(const void* data, size_t size, size_t nmemb, void* userdata);
res_t post(post_t data);
res_t get(char* preCompiledURL);
size_t header_cb(const char* buffer, size_t size, size_t nitems,
                 void* userdata);
void destroyRes(res_t res);
static res_t request(req_t req);

#endif /* request.h */