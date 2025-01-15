#include <curl/curl.h>
#include <log.h>
#include <request.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char* cc_tkn;

size_t
mem_cb(const void* data, size_t size, size_t nmemb, void* userdata)
{
  size_t realsize = size * nmemb;
  res_t* mem = (res_t*)userdata;

  char* ptr = realloc(mem->response_body, mem->size + realsize + 1);

  Assert(ptr, "out of memory.");

  mem->response_body = ptr;
  memcpy(mem->response_body + mem->size, data, realsize);
  mem->size += realsize;
  mem->response_body[mem->size] = 0;

  return realsize;
}

res_t
get(char* preCompiledURL)
{
  char auth[1536];
  memset(auth, '\0', sizeof(auth));
  sprintf(auth, "Authorization: Bearer %s", cc_tkn);
  char** headers = (char**)calloc(3, sizeof(char*));
  headers[0] = "Content-Type: application/json";
  headers[1] = "Accept: application/json";
  headers[2] = auth;

  req_t req = {
    .cb = mem_cb,
    .followLocation = 1,
    .headers = headers,
    .headerNum = 3,
    .request_type = GET,
    .targetVolume = MEMORY,
    .URL = preCompiledURL,
    .use_ssl = 1,
  };

  res_t res = request(req);
  Assert(res.status_code == 200, "failed to get response with code %zu",
         res.status_code);
  free(headers);
  return res;
}

res_t
post(post_t data)
{
  req_t req = {
    .body = data.preCompiledData,
    .cb = (data.targetVolume == _FILE)
              ? (size_t(*)(const void*, size_t, size_t, void*))fwrite
              : mem_cb,
    .filename = data.filename,
    .followLocation = 1,
    .headers = data.headers,
    .headerNum = data.headerNum,
    .request_type = POST,
    .targetVolume = data.targetVolume,
    .URL = data.preCompiledURL,
    .use_ssl = 1,
  };

  res_t res = request(req);
  Assert(res.status_code == 200, "failed to get response with code %zu",
         res.status_code);

  return res;
}

static res_t
request(req_t req)
{
  struct curl_slist* list = NULL;
  CURL* eh = curl_easy_init();
  Assert(eh, "failed to init curl");
  curl_easy_setopt(eh, CURLOPT_URL, req.URL);
  curl_easy_setopt(eh, CURLOPT_USERAGENT, req.user_agent);
  for(int i = 0; i < req.headerNum; i++)
    {
      list = curl_slist_append(list, req.headers[i]);
    }
  curl_easy_setopt(eh, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, req.use_ssl);
  curl_easy_setopt(eh, CURLOPT_TIMEOUT, (req.timeout) ? 3 : req.timeout);
  curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, req.followLocation);
  switch(req.request_type)
    {
    case GET:
      // Nothing Special.
      break;
    case POST:
      curl_easy_setopt(eh, CURLOPT_POST, 1);
      curl_easy_setopt(eh, CURLOPT_POSTFIELDS, req.body);
      curl_easy_setopt(eh, CURLOPT_POSTFIELDSIZE, strlen(req.body));
      break;
    default:
      // Not implemented.
      break;
    }
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, req.cb);
  res_t res = { 0 };
  FILE* fp;
  switch(req.targetVolume)
    {
    case MEMORY:
      curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void*)&res);
      break;
    case _FILE:
      fp = fopen(req.filename, "w+");
      Assert(fp, "failed to open file.");
      curl_easy_setopt(eh, CURLOPT_WRITEDATA, fp);
      break;
    }
  // curl_easy_setopt(eh, CURLOPT_VERBOSE, 1);
  CURLcode ins = curl_easy_perform(eh);
  Assert(ins == CURLE_OK, "curl finds error while performing");
  curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &res.status_code);
  switch(req.targetVolume)
    {
    case MEMORY:
      // Nothing Special.
      break;
    case _FILE:
      fclose(fp);
      break;
    default:
      // Not implemented.
      break;
    }
  curl_slist_free_all(list);
  curl_easy_cleanup(eh);
  return res;
}

size_t
header_cb(const char* buffer, size_t size, size_t nitems, void* userdata)
{
  char* content_disposition = strstr(buffer, "content-disposition:");
  if(content_disposition == NULL)
    {
      return nitems * size;
    }
  char* filename = strstr(buffer, "filename=\"");
  if(filename == NULL)
    {
      return nitems * size;
    }
  filename += strlen("filename=\"");
  int64_t i = 0;
  char* buf = (char*)userdata;
  while(*(filename + i) != '"')
    {
      *(buf + i) = *(filename + i);
      i++;
    }
  *(buf + i) = '\0';
  return nitems * size;
}

void
destroyRes(res_t res)
{
  if(res.response_body)
    {
      free(res.response_body);
    }
}