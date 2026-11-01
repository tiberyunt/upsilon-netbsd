/* Native spinlock operations with acquire/release fences. SPDX-License-Identifier: BSD-2-Clause */
#ifndef _UPSILON_LOCK_H_
#define _UPSILON_LOCK_H_
static inline int __SIMPLELOCK_LOCKED_P(const __cpu_simple_lock_t *p){return *p!=0;}
static inline int __SIMPLELOCK_UNLOCKED_P(const __cpu_simple_lock_t *p){return *p==0;}
static inline void __cpu_simple_lock_init(__cpu_simple_lock_t *p){*p=0;}
static inline void __cpu_simple_lock_clear(__cpu_simple_lock_t *p){*p=0;}
static inline void __cpu_simple_lock_set(__cpu_simple_lock_t *p){*p=1;}
static inline int __cpu_simple_lock_try(__cpu_simple_lock_t *p){
 unsigned n=1;__asm__ volatile("FENCE\n\tSWAP.l %0,(%1)\n\tFENCE":"+r"(n):"r"(p):"memory");return n==0;
}
static inline void __cpu_simple_lock(__cpu_simple_lock_t *p){while(!__cpu_simple_lock_try(p)){};}
static inline void __cpu_simple_unlock(__cpu_simple_lock_t *p){__asm__ volatile("FENCE":::"memory");*p=0;}
#endif
