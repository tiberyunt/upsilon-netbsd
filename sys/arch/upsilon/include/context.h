/* Upsilon MD context interfaces. SPDX-License-Identifier: BSD-2-Clause */
#ifndef _UPSILON_CONTEXT_H_
#define _UPSILON_CONTEXT_H_
#include <machine/frame.h>
#ifdef _KERNEL
struct lwp;
void ups_cpu_init(void);
int ups_lwp_prepare(struct lwp *, void *, size_t, void (*)(void *), void *);
int cpu_lwp_setprivate(struct lwp *, void *);
int ups_fault_recover(struct trapframe *,unsigned long,unsigned long);
void ups_md_trap_enter(struct trapframe *,unsigned long);
void ups_md_trap_exit(const struct trapframe *,unsigned long);
void ups_lwp_trampoline(void),ups_fork_trampoline(void);
void ups_return_user(struct trapframe *) __attribute__((noreturn));
vaddr_t cpu_lwp_pc(struct lwp *);
#endif
#endif
