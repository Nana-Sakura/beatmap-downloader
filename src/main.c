#include <apiv2.h>
#include <assert.h>
#include <cJSON.h>
#include <cli.h>
#include <commons.h>
#include <oauth.h>
#include <request.h>
#include <schedule.h>
#include <stdlib.h>
#include <uv.h>

char* cc_tkn;
unsigned clientID = 24548;
const char* clientSec = "8ZNaZq1uUo3FF3IHTOQMMXBPicf1Hcj2I1RmdBhY";
const size_t mapSize = 2 * 1024 * 1024;
u8 hashMap[mapSize];
uv_work_t req[100];

/*
 *  Consider: IF ID EX MM
 *  Fetch: User input
 *    - Get: uid      wv.bind()
 *    - Get: mode     ''
 *    - Get: range    ''
 *    - ... And More
 *    Consider CLI & GUI(webview)   UIC.render
 *  Decode: Beatmap to fetch
 *    - Get: CC_Token         getCCTkn
 *    - Get: ScoreList        getScoreList
 *    - Get: Downloaded Maps  HashMap & Read DataBase
 *    - Set: UVpresq          UV_Work giveout
 *    - ... And More
 *
 *  Execute: Utilize UV
 *    - UV Get: BeatMaps      UV.queueWork()
 *    - ... And More
 *
 *  Return: UV Status
 */

int
main(int argc, const char** argv)
{
  // UIC
  get_t data = userInput();
  // Get: CC_Token
  readToken();
  // Get: ScoreList
  res_t res = getScoreList(data);
  // Get: Downloaded Maps
  memset(hashMap, 0, mapSize);
  i64 fsize = getFileSize("hashMap");
  if(fsize > 0)
    {
      assert(fsize == mapSize * sizeof(u8));
      FILE* fp = fopen("hashMap", "r");
      fread(hashMap, sizeof(u8), fsize, fp);
      fclose(fp);
    }
  // Set: UV Presqs
  uv_loop_t* loop = uv_default_loop();
  memset(req, 0, sizeof(req));
  // Get: Parse JSON return msg
  cJSON* root = cJSON_Parse(res.response_body);
  i64 itemCount = cJSON_GetArraySize(root);
  size_t counter = 0;
  for(i64 i = 0; i < itemCount; i++)
    {
      // Get: Array[i].beatmap.beatmapset_id
      cJSON* instance = cJSON_GetArrayItem(root, i);
      cJSON* prop_beatmap = cJSON_GetObjectItem(instance, "beatmap");
      cJSON* prop_sid = cJSON_GetObjectItem(prop_beatmap, "beatmapset_id");
      // Always cleaned up at postDownload()
      uvReqData* data = (uvReqData*)calloc(1, sizeof(uvReqData));
      data->sid = (i64)cJSON_GetNumberValue(prop_sid);

      // Check: HashMap, DynamicExtend(X) staticSize(Yes) -> simplicity
      i64 index = data->sid / 8;
      i64 offset = data->sid % 8;
      if((u8)hashMap[index] & (u8)(0x1 << offset))
        {
          continue;
        }
      hashMap[index] |= (u8)(0x1 << offset);
      req[counter].data = (void*)data;
      uv_queue_work(loop, &req[counter], downloadMap, postDownload);
      counter++;
    }

  if(counter == 0)
    {
      return 0;
    }
  int uvStatus = uv_run(loop, UV_RUN_DEFAULT);

  FILE* fp = fopen("hashMap", "w+");
  size_t Res = fwrite(hashMap, sizeof(u8), mapSize, fp);
  assert(Res == mapSize);
  fclose(fp);
  return uvStatus;
}