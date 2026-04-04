#ifndef __KDBG_H__
#define __KDBG_H__

#include <boot/bootInfo.h>

/**
 * Parses kernel file to create internal symbol lookup table
 */
void init_kdbg(FileData *kernelFile);

/**
 * Uses file data from `init_kdbg` to translate instruction pointer to symbol name and offset
 * @param rip The address pointer to query info for
 * @param symbol OUT
 * @param offset OUT
 * @returns 1 on sucess, 0 on failure
 */
int get_symbol_offset(uint64_t rip, const char **symbol, uint64_t *offset);

void print_stack_trace(uint64_t rbp);

#endif
