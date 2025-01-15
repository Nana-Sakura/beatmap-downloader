#include <apiv2.h>
#include <stdio.h>
#include <string.h>

res_t
getScoreList(get_t data)
{
  char url[256];
  memset(url, 0, sizeof(url));
  sprintf(url,
          "https://osu.ppy.sh/api/v2/users/%d/scores/"
          "best?legacy_only=%d&include_fails=0&mode=%s&limit=100&offset=0",
          data.uid, data.legacy_only, data._mode);
  return get(url);
}
