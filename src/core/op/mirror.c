#include <curl/curl.h>
#include <log.h>
#include <mirror.h>
#include <stdio.h>
#include <stdlib.h>

res_t
getBeatMap(get_t data)
{
  char* url = (char*)calloc(256, sizeof(char));
  sprintf(url, "https://txy1.sayobot.cn/beatmaps/download/novideo/%d",
          data.sid);
  CURL* eh = curl_easy_init();
  if(!eh)
    {
      assert(0);
    }
  char* escaped_filename = (char*)calloc(1024, sizeof(escaped_filename));
  char* tmp_filename = (char*)calloc(256, sizeof(char));
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

  free(escaped_filename);
  free(tmp_filename);
  free(url);
  return res;
}