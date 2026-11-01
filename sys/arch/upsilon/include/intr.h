/* Upsilon hardware IPLs. SPDX-License-Identifier: BSD-2-Clause */
#ifndef _UPSILON_INTR_H_
#define _UPSILON_INTR_H_
#define IPL_NONE 0
#define IPL_SOFTCLOCK 1
#define IPL_SOFTBIO 2
#define IPL_SOFTNET 3
#define IPL_SOFTSERIAL 4
#define IPL_VM 5
#define IPL_SCHED 6
#define IPL_HIGH 7
#define IPL_SAFEPRI IPL_SOFTSERIAL
#define _IPL_N 8
#define IST_NONE 0
#define IST_PULSE 1
#define IST_EDGE 2
#define IST_LEVEL 3
typedef int ipl_t;
typedef struct { ipl_t _ipl; } ipl_cookie_t;
#ifdef _KERNEL
int splraise(int);
void splx(int);
int splhigh(void),splsched(void),splvm(void),splsoftclock(void),splsoftbio(void),splsoftnet(void),splsoftserial(void);
void spl0(void);
static inline ipl_cookie_t makeiplcookie(ipl_t n){return (ipl_cookie_t){n};}
static inline int splraiseipl(ipl_cookie_t c){return splraise(c._ipl);}
#endif
#endif
