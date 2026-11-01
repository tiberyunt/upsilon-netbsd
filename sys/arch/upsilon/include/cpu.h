/* Single-CPU NetBSD state. SPDX-License-Identifier: BSD-2-Clause */
#ifndef _UPSILON_CPU_H_
#define _UPSILON_CPU_H_
#if defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/cpu_data.h>
#include <machine/frame.h>
struct cpu_info {
 struct cpu_data ci_data;
 struct lwp *ci_curlwp,*ci_onproc;
 cpuid_t ci_cpuid;
 unsigned int ci_cpl,ci_intr_depth;
 int ci_mtx_count,ci_mtx_oldspl;
 volatile int ci_want_resched;
};
struct clockframe { vaddr_t cf_pc; register_t cf_psr; int cf_intr_depth; };
#define CLKF_PC(cf) ((cf)->cf_pc)
#define CLKF_USERMODE(cf) (((cf)->cf_psr&PSR_MODE)==PSR_USER)
#define CLKF_INTR(cf) ((cf)->cf_intr_depth>1)
#endif
#ifdef _KERNEL
extern struct cpu_info cpu_info_store[1];
static inline struct cpu_info *curcpu(void){struct cpu_info *c;__asm__ volatile("MFSR SCRATCH0,%0":"=r"(c));return c;}
#define curlwp (curcpu()->ci_curlwp)
#define curpcb ((struct pcb *)lwp_getpcb(curlwp))
static inline cpuid_t cpu_number(void){return 0;}
#define CPU_INFO_ITERATOR unsigned int
#define CPU_INFO_FOREACH(i,c) (i)=0,(c)=&cpu_info_store[0];(i)<1;(i)++
#define CPU_IS_PRIMARY(c) ((c)==&cpu_info_store[0])
#endif
#endif
