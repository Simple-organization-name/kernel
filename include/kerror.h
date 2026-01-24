#ifndef __KERROR_H__
#define __KERROR_H__

#include "kterm.h"

#define PRINT_ERR(format, ...) kprintf("[ERROR] " format, ##__VA_ARGS__)

#endif