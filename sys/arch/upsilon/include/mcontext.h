/* Provisional Upsilon signal context layout. SPDX-License-Identifier: BSD-2-Clause */
#ifndef _UPSILON_MCONTEXT_H_
#define _UPSILON_MCONTEXT_H_
#include <machine/int_types.h>
#define _UC_TLSBASE 0x00080000
#define _UC_SETSTACK 0x00010000
#define _UC_CLRSTACK 0x00020000
#define _NGREG 34
#define _REG_SP 31
#define _REG_PSR 32
#define _REG_PC 33
typedef __int64_t __greg_t;
typedef __greg_t __gregset_t[_NGREG];
typedef struct { __uint64_t __f[32]; __uint64_t __fpsr; } __fpregset_t;
typedef struct { __gregset_t __gregs; __fpregset_t __fpregs; __uint64_t __tp; } mcontext_t;
#endif
