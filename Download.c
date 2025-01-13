#include <assert.h>
#include <cJSON.h>
#include <curl/curl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define i64 int64_t
#define u8 unsigned char

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
  unsigned mods;
  unsigned mode;
  char* _mode;
  char* checksum;
} get_t;

static i64 getFileSize(char* path);
static i64 getLastAccessInterval(char* filename);
static res_t getCcToken();
static size_t mem_cb(const void* data, size_t size, size_t nmemb,
                     void* userdata);
static res_t post(post_t data);
static res_t request(req_t req);
static void parseToken(res_t tkn);
static void readToken();
static res_t getScoreList(get_t data);
static size_t header_cb(const char* buffer, size_t size, size_t nitems,
                        void* userdata);
static res_t getBeatMap(get_t data);
static void destroyRes(res_t res);

static char* cc_tkn;
static unsigned clientID = 24548;
static const char* clientSec = "8ZNaZq1uUo3FF3IHTOQMMXBPicf1Hcj2I1RmdBhY";
static u8 hashMap[10 * 1024 * 1024];

static i64
getFileSize(char* path)
{
  struct stat buf;
  int res = stat(path, &buf);
  return res == 0 ? buf.st_size : -1;
}

static i64
getLastAccessInterval(char* filename)
{
  struct stat buf;
  int res = stat(filename, &buf);
  if(res)
    {
      return -1;
    }
  else
    {
      time_t lastMod = buf.st_mtime;
      time_t now = time(NULL);
      return now - lastMod;
    }
}

static res_t
getCcToken()
{
  char** headers = (char**)calloc(2, sizeof(char*));
  int headerNum = 2;
  headers[0] = "Accept: application/json";
  headers[1] = "Content-Type: application/x-www-form-urlencoded";

  char data[256];
  memset(data, '\0', sizeof(data));
  sprintf(data,
          "client_id=%u&client_secret=%s&grant_type=client_credentials&scope="
          "public",
          clientID, clientSec);
  post_t _post = {
    .headers = headers,
    .preCompiledData = data,
    .preCompiledURL = "https://osu.ppy.sh/oauth/token",
    .targetVolume = MEMORY,
    .headerNum = headerNum,
  };
  res_t res = post(_post);
  free(headers);
  return res;
}

static size_t
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

static res_t
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

static res_t
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

static void
parseToken(res_t tkn)
{
  assert(tkn.size);
  /*
   *  {
   *    "access_token": "verylongstring",
   *    "expires_in": 86400,
   *    "token_type": "Bearer"
   *  }
   *
   */

  cJSON* root = cJSON_Parse(tkn.response_body);
  cJSON* act = cJSON_GetObjectItem(root, "access_token");
  char* stkn = strdup(cJSON_GetStringValue(act));
  FILE* fp = fopen("cc_tkn", "w+");
  assert(fp);
  fwrite((void*)stkn, sizeof(char), strlen(stkn), fp);
  fclose(fp);
  free(stkn);
  cJSON_Delete(root);
}

static void
readToken()
{
  int64_t fsize = getFileSize("cc_tkn");
  int64_t lastModInt = getLastAccessInterval("cc_tkn");
  if(fsize == -1 || lastModInt == -1 || lastModInt > 60 * 60 * 24)
    {
      res_t res = getCcToken();
      assert(res.status_code == 200);
      parseToken(res);
      fsize = getFileSize("cc_tkn");
      assert(fsize != -1);
      destroyRes(res);
    }
  FILE* fp = fopen("cc_tkn", "r");
  assert(fp);
  if(cc_tkn)
    {
      free(cc_tkn);
    }
  cc_tkn = (char*)calloc(fsize + 1, sizeof(char));
  fread(cc_tkn, sizeof(char), fsize, fp);
  fclose(fp);
}

static res_t
getScoreList(get_t data)
{
  char url[256];
  memset(url, 0, sizeof(url));
  sprintf(url,
          "https://osu.ppy.sh/api/v2/users/%d/scores/"
          "best?legacy_only=0&include_fails=0&mode=osu&limit=100&offset=0",
          data.uid);
  return get(url);
}

static size_t
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
  i64 i = 0;
  char* buf = (char*)userdata;
  while(*(filename + i) != '"')
    {
      *(buf + i) = *(filename + i);
      i++;
    }
  *(buf + i) = '\0';
  // puts(buf);
  return nitems * size;
}

static res_t
getBeatMap(get_t data)
{
  char url[256];
  memset(url, 0, sizeof(url));
  sprintf(url, "https://txy1.sayobot.cn/beatmaps/download/novideo/%d",
          data.sid);
  CURL* eh = curl_easy_init();
  if(!eh)
    {
      assert(0);
    }
  char escaped_filename[1024];
  memset(escaped_filename, 0, sizeof(escaped_filename));

  char tmp_filename[256];
  sprintf(tmp_filename, "%d.osz", data.sid);
  FILE* fp = fopen(tmp_filename, "w+");

  res_t res = { 0 };
  curl_easy_setopt(eh, CURLOPT_HEADERDATA, (void*)escaped_filename);
  curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, header_cb);
  curl_easy_setopt(eh, CURLOPT_URL, url);
  curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 1);
  curl_easy_setopt(eh, CURLOPT_TIMEOUT, 300);
  curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, fwrite);
  curl_easy_setopt(eh, CURLOPT_WRITEDATA, fp);
  // curl_easy_setopt(eh, CURLOPT_VERBOSE, 1);
  CURLcode inst = curl_easy_perform(eh);
  Assert(inst == CURLE_OK, "curl finds error while performing");
  curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &res.status_code);
  fclose(fp);

  // Get: Filename(Content-Disposition)
  char* filename = curl_easy_unescape(eh, escaped_filename, 0, NULL);
  assert(filename);
  int r = rename(tmp_filename, filename);
  assert(r == 0);
  curl_free(filename);
  curl_easy_cleanup(eh);
  return res;
}

static void
destroyRes(res_t res)
{
  if(res.response_body)
    {
      free(res.response_body);
    }
}

int
main(void)
{
  int uid;
  printf("Input the uid you want to get beatmap from: ");
  scanf("%d", &uid);
  readToken();
  get_t data = {
    .uid = uid,
  };
  res_t res = getScoreList(data);
  // printf("%s\n", res.response_body);

  // HashMap Specifics
  // In order to Dynamically Extend, will get the biggest SID
  // in stack.

  // Or in sake of simplicity, considering making that to be 10MB.
  i64 fsize = getFileSize("hashMap");
  if(fsize > 0)
    {
      assert(fsize == 10 * 1024 * 1024 * sizeof(u8));
      FILE* fp = fopen("hashMap", "r");
      fread(hashMap, sizeof(u8), fsize, fp);
      fclose(fp);
    }

  int stack[100];
  int sp = 0;
  memset(stack, 0, sizeof(stack));

  // Get: Parse JSON return msg
  cJSON* root = cJSON_Parse(res.response_body);
  i64 itemCount = cJSON_GetArraySize(root);
  for(i64 i = 0; i < itemCount; i++)
    {
      // Get: Array[i].beatmap.beatmapset_id
      cJSON* instance = cJSON_GetArrayItem(root, i);
      cJSON* prop_beatmap = cJSON_GetObjectItem(instance, "beatmap");
      cJSON* prop_sid = cJSON_GetObjectItem(prop_beatmap, "beatmapset_id");
      i64 sid = (i64)cJSON_GetNumberValue(prop_sid);

      // Check: HashMap, DynamicExtend
      i64 index = sid / 8;
      i64 offset = sid % 8;

      // Has been downloaded
      if((u8)hashMap[index] & (u8)(0x1 << offset))
        {
          continue;
        }
      hashMap[index] |= (u8)(0x1 << offset);
      stack[sp++] = sid;
    }

  // Get: https://txy1.sayobot.cn/beatmaps/download/[novideo]or[full]/SID
  while(sp)
    {
      int sid = stack[--sp];
      get_t checkOut = {
        .sid = sid,
      };
      getBeatMap(checkOut);
    }

  // Output: HashMap Write Back
  FILE* f = fopen("hashMap", "w+");
  size_t wRes = fwrite(hashMap, sizeof(u8), 10 * 1024 * 1024, f);
  assert(wRes == 10 * 1024 * 1024);
  destroyRes(res);
}