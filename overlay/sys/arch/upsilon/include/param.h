/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef _UPSILON_PARAM_H_
#define _UPSILON_PARAM_H_
#define _MACHINE upsilon
#define MACHINE "upsilon"
#define MACHINE_ARCH "upsilon"
#define MID_MACHINE MID_UPSILON
#define ALIGNBYTES __ALIGNBYTES
#define ALIGNED_POINTER(p,t) ((((unsigned long)(p)) & (__alignof__(t)-1))==0)
#define PGSHIFT 12
#define NBPG (1UL << PGSHIFT)
#define PGOFSET (NBPG-1)
#define PAGE_SHIFT PGSHIFT
#define PAGE_SIZE NBPG
#define PAGE_MASK PGOFSET
#define KERNBASE 0xffff800000000000UL
#define UPAGES 4
#define USPACE (UPAGES*NBPG)
#define USPACE_ALIGN 0
#define UAREA_PCB_OFFSET 0
#endif
