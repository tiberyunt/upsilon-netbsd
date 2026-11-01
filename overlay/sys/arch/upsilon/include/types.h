/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _UPSILON_TYPES_H_
#define _UPSILON_TYPES_H_
#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <machine/int_types.h>
typedef __int64_t __register_t;
#if defined(_KERNEL) || defined(_STANDALONE) || defined(_KMEMUSER)
typedef __uint64_t paddr_t,psize_t,vaddr_t,vsize_t;
typedef __int64_t register_t;
typedef __uint64_t uregister_t;
#define PRIxPADDR "lx"
#define PRIxPSIZE "lx"
#define PRIuPSIZE "lu"
#define PRIxVADDR "lx"
#define PRIxVSIZE "lx"
#define PRIuVSIZE "lu"
#endif
#if defined(_KERNEL) || defined(_KMEMUSER)
typedef struct ups_label { __uint64_t lb_reg[15],lb_sp,lb_pc,lb_psr,lb_tp; } label_t;
#endif
typedef unsigned int __cpu_simple_lock_nv_t;
#define __HAVE_CPU_DATA_FIRST 1
#define __HAVE_ATOMIC64_OPS 1
#define __SIMPLELOCK_LOCKED 1
#define __SIMPLELOCK_UNLOCKED 0
#endif
