/* Kernel switch state, distinct from the full exception trapframe. BSD-2-Clause */
#ifndef _UPSILON_PCB_H_
#define _UPSILON_PCB_H_
#include <sys/types.h>
struct ups_switchframe {
 uint64_t sf_saved[15]; /* R16..R30 */
 uint64_t sf_sp;       /* Points to the suspended call's return address. */
 uint64_t sf_psr;
 uint64_t sf_tp;
 uint64_t sf_ptbr0;
 uint64_t sf_asid;
};
struct pcb {
 struct ups_switchframe pcb_ctx;
 uint64_t pcb_fpregs[32],pcb_fpsr;
};
#endif
