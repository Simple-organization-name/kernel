#ifndef __KTHREAD_H__
#define __KTHREAD_H__

#include <idt.h>

void init_threading(void (*resume_func)());

void switch_thread();

void spawn_thread(void (*entry)(), unsigned stackPageCount);

void exit_thread();

#endif
