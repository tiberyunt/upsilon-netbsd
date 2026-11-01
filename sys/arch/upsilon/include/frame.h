/* Software saves R0..R31; hardware appends PSR and PC. BSD-2-Clause */
#ifndef _UPSILON_FRAME_H_
#define _UPSILON_FRAME_H_
#ifndef _LOCORE
#include <sys/types.h>
#define lwp_trapframe(l) ((l)->l_md.md_utf)
struct trapframe { __uint64_t tf_r[32]; __uint64_t tf_psr,tf_pc; };
#endif
#define TF_PSR 256
#define TF_PC 264
#define TF_SIZE 272
#define PSR_C 1
#define PSR_IPL 0x700
#define PSR_USER 0x2000
#define PSR_MODE 0x3000
#define PSR_FPD 0x10000
#define PSR_UA 0x20000
#endif
