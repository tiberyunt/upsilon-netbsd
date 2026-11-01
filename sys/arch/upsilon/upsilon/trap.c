/* Trap, timer and syscall bring-up tests; not the MI syscall dispatcher.
 * SPDX-License-Identifier: BSD-2-Clause */
#include "boot.h"
#include <sys/errno.h>
#include <sys/syscall.h>
volatile unsigned long ups_ticks;
/* Bootstrap IRQ callback; a full kernel will dispatch hardclock instead. */
void (*ups_clock_test_hook)(void);
void (*ups_user_exit_hook)(struct trapframe *);
unsigned long ups_user_result=~0UL;
int ups_expect_fault;
void ups_clock_start(void){ups_ticks=0;ups_compare(ups_cycles()+10000);ups_ipl(PSR_FPD);}
#include <machine/context.h>
static void dispatch(struct trapframe *tf,unsigned long cause,unsigned long address,unsigned long info){
 if(ups_fault_recover(tf,cause,info))return;
 if(cause==64){ups_ticks++;ups_compare(ups_ticks<3?ups_cycles()+10000:~0UL);if(ups_clock_test_hook)ups_clock_test_hook();return;}
 if(cause==6&&ups_expect_fault&&address==0x60000000UL){
  ups_expect_fault=0;tf->tf_pc=(unsigned long)ups_fault_resume;return;
 }
 if(cause==12&&(tf->tf_psr&PSR_MODE)==PSR_USER){
  unsigned long error=0,result=0;
  switch(tf->tf_r[0]){
   case SYS_getpid:result=1;break;
   case SYS_write:{
    unsigned long pointer=tf->tf_r[2],length=tf->tf_r[3];
    if(tf->tf_r[1]!=1&&tf->tf_r[1]!=2){error=EBADF;break;}
    char buffer[64];
    while(result<length){
     size_t n=length-result;if(n>sizeof(buffer))n=sizeof(buffer);
     error=pointer>UINTPTR_MAX-result?EFAULT:copyin((const void *)(pointer+result),buffer,n);
     if(error){if(result)error=0;break;}
     for(size_t i=0;i<n;i++)putchar((unsigned char)buffer[i]);
     result+=n;
    }
    break;
   }
   case SYS_exit:
    if(ups_user_exit_hook){ups_user_exit_hook(tf);return;}
    ups_user_result=tf->tf_r[1];tf->tf_r[31]=(unsigned long)(tf+1);tf->tf_pc=(unsigned long)ups_user_return;tf->tf_psr=PSR_FPD|PSR_IPL;return;
   default:error=ENOSYS;break;
  }
  tf->tf_r[0]=error?error:result;
  tf->tf_psr=(tf->tf_psr&~PSR_C)|(error?PSR_C:0);return;
 }
 printf("trap %lu pc=%lx address=%lx psr=%lx\n",cause,tf->tf_pc,address,tf->tf_psr);
 ups_fail("unexpected trap");
}

void ups_trap(struct trapframe *tf,unsigned long cause,unsigned long address,unsigned long info){
 ups_md_trap_enter(tf,cause);dispatch(tf,cause,address,info);ups_md_trap_exit(tf,cause);
}
