#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdarg.h>
#include <stdio.h>

#define LOG_ENABLE
#define LOG_OUTPUT_STD

void init_log_file(char *path);
int close_log_file(void);
int err(char *str,...);

#ifdef LOG_ENABLE
int debug(char *str,...);
int dump_memory(const char *buf, int count);
#else
__inline static int debug(char *str,...){}
__inline static int dump_memory(const char *buf, int count){}
#endif

#endif
