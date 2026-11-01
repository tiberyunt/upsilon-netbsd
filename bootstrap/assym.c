/* Target-produced offsets for assembly; uses the real MI LWP. BSD-2-Clause */
#include <sys/param.h>
#include <sys/lwp.h>
#include <machine/pcb.h>
_Static_assert(sizeof(label_t)==152,"fault-label size");
_Static_assert(__builtin_offsetof(struct cpu_info,ci_data)==0,"MI CPU data must come first");
#define O(t,m) __builtin_offsetof(t,m)
const unsigned long ups_assym[] __attribute__((used,section(".rodata.assym")))={
#define ASSYM(name,type,field) O(type,field),
#include "assym.def"
#undef ASSYM
};
