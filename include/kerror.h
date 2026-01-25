#ifndef __KERROR_H__
#define __KERROR_H__

#include "kterm.h"

#define PRINT_ERR(format, ...)  kprintf("[ ERROR ][%s] " format, __func__, ##__VA_ARGS__)
#define PRINT_WARN(format, ...) kprintf("[WARNING][%s] " format, __func__, ##__VA_ARGS__)

#endif