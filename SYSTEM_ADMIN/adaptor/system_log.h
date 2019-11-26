#ifndef _H_SYSTEM_LOG_
#define _H_SYSTEM_LOG_
#include <time.h>

void system_log_init(const char *path);
extern int system_log_run(void);
void system_log_info(char *format, ...);
extern int system_log_stop(void);
extern void system_log_free(void);

#endif
