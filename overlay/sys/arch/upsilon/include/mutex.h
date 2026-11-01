/* Kernel mutex storage; MI mutex implementation is not wired yet. BSD-2-Clause */
#ifndef _UPSILON_MUTEX_H_
#define _UPSILON_MUTEX_H_
#include <sys/types.h>
#include <machine/intr.h>
struct kmutex { volatile uintptr_t mtx_owner; ipl_cookie_t mtx_ipl; };
#endif
