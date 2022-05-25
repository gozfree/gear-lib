#include "pseudotcp_wrap.h"
#include <time.h>
#include <stdarg.h>

gint64 g_get_monotonic_time()
{
    int ret;
    struct timespec ts;

    ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ret != 0) {
        printf("clock_gettime failed!\n");
        return -1;
    }

    return (((int64_t) ts.tv_sec) * 1000000) + (ts.tv_nsec / 1000);
}


void g_log (const gchar   *log_domain,
       GLogLevelFlags log_level,
       const gchar   *format,
       ...)
{
  char buf[1024];
  va_list args;

  va_start (args, format);
  vsnprintf(buf, sizeof(buf)-1, format, args);
  va_end (args);
  printf("%s\n", buf);
}
