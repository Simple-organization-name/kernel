#include <kthread.h>
#include <stddef.h>
#include <attribute.h>
#include <memmap.h>
#include <buddy.h>
#include <memory.h>
#include <asm.h>

#define TSTACKS_PML4_INDEX 510
#define TSTACKS_PDPT_INDEX 507

typedef struct _ThreadContext {
    VirtAddr stack_origin;
    VirtAddr stack_current;
    unsigned stackPageCount;
    
} ThreadContext;

typedef struct _ThreadNode {
    struct _ThreadNode *prev;
    struct _ThreadNode *next;
    ThreadContext context;
} ThreadNode;

ThreadNode *threads = NULL;
static ThreadNode *nodeReserve = NULL;
static bool thread_is_init = false;
static bool previous_exited = false;

static void insertContextQueueNext(ThreadNode **queue, ThreadNode *context)
{
    if (!*queue) {
        *queue = context;
        (*queue)->next = *queue;
        (*queue)->prev = *queue;
        return;
    }
    context->prev = *queue;
    context->next = (*queue)->next;
    context->next->prev = context;
    (*queue)->next = context;
    return;
}

ThreadNode *extractContextQueue(ThreadNode **queue)
{
    if (!*queue) return NULL;
    ThreadNode *out = (*queue)->prev;
    (*queue)->prev = out->prev;
    (*queue)->prev->next = *queue;
    return out;
}

static void rotateContextQueueNext(ThreadNode **queue)
{
    if (!*queue) return;
    *queue = (*queue)->next;
}


static int fillThreadContextReserve()
{
    const int contextCount = 4096 / sizeof(ThreadNode);
    PhysAddr pa = buddyAlloc(BUDDY_4K);
    uint16_t idx[4] = { TSTACKS_PML4_INDEX, TSTACKS_PDPT_INDEX };
    if (!findEmptySlotPageIdx(PTE_PT, idx)) {
        return 1;
    }
    if (mapPage(idx, PTE_PT, pa, PTE_RW | PTE_PS)) {
        return 1;
    }
    ThreadNode **va = VA_ARRAY(idx);
    memset(va, 0, 4096);
    for (int i = 0; i < contextCount; i++) {
        insertContextQueueNext(&nodeReserve, va[i]);
    }
    return 0;
}


void init_threading(void (* resume_func)())
{
    kprintf("b$$$\n");
    if (PML4()[TSTACKS_PML4_INDEX].whole == 0) {
        kprintf("b444\n");
        PhysAddr pml4Page = buddyAlloc(BUDDY_4K);
        PML4()[TSTACKS_PML4_INDEX].whole = MAKE_PAGE_ENTRY(pml4Page, PTE_RW);
        kprintf("b555\n");
    } else if (!PML4()[TSTACKS_PML4_INDEX].present) {
        PRINT_ERR("PML4 slot for thread stacks is used but not mapped");
        CRIT_HLT();
    }
    if (PDPT(TSTACKS_PML4_INDEX)[TSTACKS_PDPT_INDEX].whole == 0) {
        kprintf("b666\n");
        PhysAddr pml4Page = buddyAlloc(BUDDY_4K);
        PDPT(TSTACKS_PML4_INDEX)[TSTACKS_PDPT_INDEX].whole = MAKE_PAGE_ENTRY(pml4Page, PTE_RW);
        kprintf("b777\n");
    } else if (!PDPT(TSTACKS_PML4_INDEX)[TSTACKS_PDPT_INDEX].present) {
        PRINT_ERR("PDPT slot for thread stacks is used but not mapped");
        CRIT_HLT();
    }
    kprintf("baaa\n");
    fillThreadContextReserve();
    kprintf("bbbb\n");
    thread_is_init = true;
    spawn_thread(resume_func, 0);
    kprintf("bccc\n");
    switch_thread();
}

extern void thread_init_stub(VirtAddr *callerStack, const VirtAddr *calledStack, void (* entry)());
extern void thread_switch_stub(VirtAddr *callerStack, const VirtAddr *calledStack);
extern void thread_ret_stub(VirtAddr *callerStack, const VirtAddr *calledStack);

void switch_thread()
{
    if (!threads || threads->next == threads || true) return;
    rotateContextQueueNext(&threads);
    thread_switch_stub(
        &threads->prev->context.stack_origin,
        &threads->context.stack_origin
    );
}

void spawn_thread(void (*entry)(), unsigned stackPageCount)
{
    if (stackPageCount == 0) stackPageCount = 10;
    if (!nodeReserve) fillThreadContextReserve();
    ThreadNode *new = extractContextQueue(&nodeReserve);
    uint16_t idx[4] = { 510, 507 };
    findEmptyRangePageIdx(PTE_PT, idx, stackPageCount + 1);
    new->context.stackPageCount = stackPageCount;
    new->context.stack_origin = (VirtAddr)VA_ARRAY(idx);
    new->context.stack_current = new->context.stack_origin + 4096*stackPageCount;
    while (stackPageCount) {
        PhysAddr pa = buddyAlloc(BUDDY_4K);
        kprintf("stack alloc at 0x%X\n", pa);
        mapPage(idx, PTE_PT, pa, PTE_RW | PTE_PS);
        idx[3]++;
        if (idx[3] == 512) {
            idx[3] = 0;
            idx[2]++;
            if (idx[2] == 512) {
                idx[2] = 0;
                idx[1]++;
                if (idx[1] == 512) {
                    idx[1] = 0;
                    idx[0]++;
                    if (idx[0] == 512) {
                        PRINT_ERR("FUCK YOU\n");
                        CRIT_HLT();
                    }
                }
            }
        }
        stackPageCount--;
    }

    insertContextQueueNext(&threads, new);
    rotateContextQueueNext(&threads);
    thread_init_stub(
        &threads->prev->context.stack_current,
        &threads->context.stack_current,
        entry
    );
}

void exit_thread()
{
    if (!threads) {
        PRINT_ERR("What the fuck dude");
        CRIT_HLT();
    }
    if (threads->next == threads) {
        PRINT_ERR("Exiting final thread; hanging");
        CRIT_HLT();
    }
    
    previous_exited = true;
    switch_thread();

    return;
}
