/* NetBSD machine-dependent LWP state. SPDX-License-Identifier: BSD-2-Clause */
#ifndef _UPSILON_PROC_H_
#define _UPSILON_PROC_H_
struct trapframe;
struct ups_label;
struct mdlwp {
 struct trapframe *md_utf;
 struct ups_label *md_onfault;
 volatile int md_astpending;
};
struct mdproc { void (*md_syscall)(struct trapframe *); };
#ifdef _KERNEL
#define LWP0_CPU_INFO (&cpu_info_store[0])
#endif
#endif
