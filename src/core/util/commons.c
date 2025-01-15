#include <commons.h>
#include <stddef.h>
#include <sys/stat.h>
#include <time.h>

i64
getFileSize(char* path)
{
  struct stat buf;
  int res = stat(path, &buf);
  return res == 0 ? buf.st_size : -1;
}

i64
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