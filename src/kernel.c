#include "boot.h"

typedef void BootInfo;

_Noreturn void kernelMain(BootInfo* bootInfo);

_Noreturn void _start(BootInfo* bootInfo)
{
    
    while(1);
    kernelMain(bootInfo);
}

_Noreturn void kernelMain(BootInfo* bootInfo)
{

    while(1 || bootInfo);
}