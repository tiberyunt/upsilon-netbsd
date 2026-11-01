/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef UPS_BOOT_H
#define UPS_BOOT_H
#include <sys/types.h>
#include <machine/frame.h>
#include <machine/pte.h>
#ifdef _KERNEL
#include <sys/systm.h>
void putchar(int);
#else
#include <lib/libsa/stand.h>
#endif
#define UPS_KERNBASE 0xffff800000000000UL
#define UPS_USER_TEXT 0x40000000UL
#define UPS_USER_STACK 0x40002000UL
extern volatile unsigned long ups_ticks;
extern unsigned long ups_memory,ups_fdt,ups_user_result;
void ups_main(unsigned long);
void ups_trap(struct trapframe *,unsigned long,unsigned long,unsigned long);
void ups_pmap_bootstrap(unsigned long);
unsigned long ups_pmap_clone_bootstrap(void);
void ups_pmap_enter_root(unsigned long,unsigned long,unsigned long,unsigned long);
void ups_pmap_enter(unsigned long,unsigned long,unsigned long);
void ups_clock_start(void);
void ups_pmap_tests(void),ups_atomic_tests(void),ups_fork_tests(void);
extern void (*ups_user_exit_hook)(struct trapframe *);
extern int ups_test_returning_kthread;
extern void (*ups_clock_test_hook)(void);
void ups_cpu_init(void),ups_context_tests(void),ups_copy_tests(void),ups_user_tls_check(void);
void ups_fault_probe(void),ups_fault_resume(void);
void ups_enter_user(void),ups_user_return(void);
void ups_fail(const char *) __attribute__((noreturn));
extern char ups_user_start[],ups_user_end[],ups_user_stack[];
extern unsigned long ups_vectors[];
static inline unsigned long ups_cycles(void){unsigned long n;__asm__ volatile("MFSR CYCLES,%0":"=r"(n));return n;}
static inline void ups_compare(unsigned long n){__asm__ volatile("MTSR %0,TIMECMP"::"r"(n):"memory");}
void ups_md_psr(unsigned long);
static inline void ups_ipl(unsigned long n){ups_md_psr(n);}
#endif
