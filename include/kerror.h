#ifndef __KERROR_H__
#define __KERROR_H__

#include "kterm.h"

#define PRINT_ERR(format, ...)  kprintf("[ ERROR ][%s:%u] " format, __func__, __LINE__, ##__VA_ARGS__)
#define PRINT_WARN(format, ...) kprintf("[WARNING][%s:%u] " format, __func__, __LINE__, ##__VA_ARGS__)

#endif
