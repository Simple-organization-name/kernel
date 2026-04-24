#include <stddef.h>
#include <kdbg.h>
#include <kterm.h>
#include <asm.h>
#include <attribute.h>

#include <elf.h>

typedef struct {
    uint64_t addr;
    const char *name;
} OffsetToName;

__attribute_maybe_unused__
static OffsetToName symbolMap[1024];
__attribute_maybe_unused__
static int symbolCount = 0;

static inline int streq(const char *a, const char *b)
{
    while (*a && *a == *b) a++, b++;
    return *a == *b;
}

void init_kdbg(FileData *kernelFile)
{
    uint8_t *data = kernelFile->data;
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)data;
    Elf64_Shdr *symHdr = NULL;
    Elf64_Shdr *sHdrs = (Elf64_Shdr *)(data + ehdr->e_shoff);

    // find symbol table
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (sHdrs[i].sh_type != SHT_SYMTAB) continue;
        kprintf("Found kernel symbol table at offset data+0x%X, of size %D; assuming single\n", sHdrs[i].sh_offset, sHdrs[i].sh_size);
        symHdr = &sHdrs[i];
        break;
    }

    Elf64_Shdr *strHdr = &sHdrs[symHdr->sh_link];
    if (strHdr->sh_type != SHT_STRTAB) {
        kprintf("baaaaaaaaah !!!\n");
        CRIT_HLT();
    } else {
        kprintf("phewww\n");
    }

    char *strings = (char *)(data + strHdr->sh_offset);

    if (!symHdr) {
        PRINT_ERR("Could not find kernel symbol table\n");
        CRIT_HLT();
    }

    Elf64_Sym *sym = (Elf64_Sym *)(data + symHdr->sh_offset);
    int symCount = symHdr->sh_size / symHdr->sh_entsize;
    uint64_t loadOffset = 0;
    for (int i = 1; i < symCount; i++)
    {
        // if (ELF64_ST_TYPE(sym[i].st_info) != STT_FUNC) continue;
        const char *symName = &strings[sym[i].st_name];
        // kprintf("Symbol #%d : %s\n", i, symName);
        symbolMap[symbolCount].addr = sym[i].st_value;
        symbolMap[symbolCount].name = symName;
        symbolCount++;
        if (streq(symName, "init_kdbg")) {
            loadOffset = (long)init_kdbg - sym[i].st_value;
        }
    }
    
    if (!loadOffset) {
        PRINT_WARN("Could not find achor symbol [init_kdbg]; Functionality cut off\n");
        symbolCount = 0;
        return;
    }

    for (int i = 0; i < symbolCount; i++)
    {
        symbolMap[i].addr += loadOffset;
    }
}

int get_symbol_offset(uint64_t rip, const char **symbol, uint64_t *offset)
{
    const char *bestSymbol = "???";
    uint64_t bestOffset = rip;

    for (int i = 0; i < symbolCount; i++)
    {
        if (symbolMap[i].addr > rip) continue;
        uint64_t off = rip - symbolMap[i].addr;
        if (off < bestOffset) {
            bestSymbol = symbolMap[i].name;
            bestOffset = off;
        }
    }

    *symbol = bestSymbol;
    *offset = bestOffset;

    return 0;
}

void print_stack_trace(uint64_t rbp)
{
    struct _stackTrace {
        struct _stackTrace *rbp;
        uint64_t rip;
    } *trace = (struct _stackTrace *)rbp;
    for (int i = 0; i < 10000 && trace->rbp; i++) {
        const char *name;
        uint64_t offset;
        get_symbol_offset(trace->rip, &name, &offset);
        kprintf("RIP=0x%X => %s + 0x%X\n", trace->rip, name, offset);
        trace = trace->rbp;
    }
}
