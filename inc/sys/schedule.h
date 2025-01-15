#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <commons.h>
#include <uv.h>

typedef struct __uvReqData
{
  i64 sid;
} uvReqData;

void downloadMap(uv_work_t* req);
void postDownload(uv_work_t* req, int status);

#endif /* schedule.h */