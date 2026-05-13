#ifndef __LOCK_H__
#define __LOCK_H__

#include <stdatomic.h>
#include <kerror.h>

typedef volatile atomic_flag kernel_lock;

#define LOCK_INIT ATOMIC_FLAG_INIT

#define LOCK_SPINLOCK(lock) while (atomic_flag_test_and_set_explicit((lock), memory_order_acquire)) __builtin_ia32_pause()
#define LOCK_RELEASE(lock) atomic_flag_clear_explicit((lock), memory_order_release)

#define LOCK_SPINLOCK_IRQSAVE(lock, irq_state) \
    do { \
        __asm__ volatile( \
            "pushfq\n" \
            "pop %0\n" \
            "cli" : "=rm"(irq_state) :: "memory"); \
        LOCK_SPINLOCK(lock); \
    } while (0)

#define LOCK_RELEASE_IRQRESTORE(lock, irq_state) \
    do { \
        LOCK_RELEASE(lock); \
        __asm__ volatile( \
            "push %0\n" \
            "popfq" :: "rm"(irq_state) : "memory", "cc"); \
    } while (0)

#endif