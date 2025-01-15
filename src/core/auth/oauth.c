#include <assert.h>
#include <cJSON.h>
#include <commons.h>
#include <oauth.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern size_t clientID;
extern const char* clientSec;
extern char* cc_tkn;

res_t
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

void
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

void
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