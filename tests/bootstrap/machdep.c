/* First Upsilon NetBSD source integration probe. SPDX-License-Identifier: BSD-2-Clause */
#include "boot.h"
#include <libfdt.h>
_Static_assert(sizeof(struct trapframe)==272,"trap frame size");
_Static_assert(__builtin_offsetof(struct trapframe,tf_psr)==256,"hardware PSR slot");
_Static_assert(__builtin_offsetof(struct trapframe,tf_pc)==264,"hardware PC slot");
unsigned long ups_memory,ups_fdt;
extern int ups_expect_fault;
static volatile unsigned char *const uart=(void *)0xffff0400UL;
void putchar(int c){while(!(uart[5]&0x20)){}uart[0]=(unsigned char)c;}
void ups_fail(const char *why){printf("BRINGUP FAILED: %s\n",why);__asm__ volatile("HALT");for(;;){}}
static unsigned long value64(const void *p){const unsigned char *b=p;unsigned long n=0;for(int i=0;i<8;i++)n=(n<<8)|b[i];return n;}
void ups_main(unsigned long dtb){
 ups_fdt=dtb;
 uart[3]=0x80;uart[0]=1;uart[1]=0;uart[3]=3;uart[2]=1;
 printf("Upsilon NetBSD 11.0 bootstrap probe (not a full kernel)\n");
 if(fdt_check_header((void *)dtb))ups_fail("invalid FDT header");
 int n=fdt_path_offset((void *)dtb,"/memory@0"),len;
 const void *reg=fdt_getprop((void *)dtb,n,"reg",&len);
 if(!reg||len!=16||value64(reg)!=0)ups_fail("invalid memory description");
 ups_memory=value64((const char *)reg+8);
 printf("FDT: %lu MiB RAM, cpu 0, ns16550 console\n",ups_memory>>20);
 n=fdt_path_offset((void *)dtb,"/chosen");
 const char *args=fdt_getprop((void *)dtb,n,"bootargs",&len);
 if(args&&len>1)printf("bootargs: %s\n",args);
 ups_test_returning_kthread=args&&len>1&&!strcmp(args,"--test-returning-kthread");
 if(ups_memory<16*1024*1024)ups_fail("16 MiB required");
 unsigned long frequency;__asm__ volatile("MFSR TIMEFREQ,%0":"=r"(frequency));
 printf("timer: %lu Hz\n",frequency);
 __asm__ volatile("MTSR %0,VBR"::"r"((unsigned long)ups_vectors):"memory");
 ups_pmap_bootstrap(ups_memory);
 if(*(const unsigned long *)(UPS_KERNBASE+dtb)!=*(const unsigned long *)dtb)ups_fail("high-half mapping");
 printf("MMU: identity + high-half alias, device mapping passed\n");
 ups_cpu_init();
 ups_expect_fault=1;ups_fault_probe();if(ups_expect_fault)ups_fail("page-fault recovery");
 printf("trap: unmapped load recovered through RTE\n");
 ups_clock_start();while(ups_ticks<3)__asm__ volatile("WAIT":::"memory");
 ups_ipl(PSR_FPD|PSR_IPL);printf("timer: %lu interrupts passed\n",ups_ticks);
 ups_context_tests();
 ups_atomic_tests();
 ups_pmap_tests();
 ups_fork_tests();
 if((unsigned long)(ups_user_end-ups_user_start)>4096)ups_fail("user text too large");
 ups_pmap_enter(UPS_USER_TEXT,(unsigned long)ups_user_start,UPS_PTE_R|UPS_PTE_X|UPS_PTE_USER);
 ups_pmap_enter(UPS_USER_STACK-4096,(unsigned long)ups_user_stack,UPS_PTE_R|UPS_PTE_W|UPS_PTE_USER);
 ups_copy_tests();
 ups_enter_user();if(ups_user_result)ups_fail("user syscall return");
 printf("user mode: write/getpid/ENOSYS/EFAULT passed\n");
 ups_user_tls_check();
 printf("BRINGUP PASS: NetBSD libsa/libfdt, MMU, traps, timer, user transition\n");
 __asm__ volatile("HALT");
}
