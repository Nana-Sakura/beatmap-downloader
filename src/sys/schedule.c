#include <mirror.h>
#include <request.h>
#include <schedule.h>
#include <stdlib.h>

void
downloadMap(uv_work_t* req)
{
  uvReqData* data = (uvReqData*)req->data;
  i64 sid = data->sid;

  get_t getData = {
    .sid = sid,
  };

  // Get: https://txy1.sayobot.cn/beatmaps/download/[novideo]or[full]/SID
  getBeatMap(getData);
}

void
postDownload(uv_work_t* req, int status)
{
  free(req->data);
}

