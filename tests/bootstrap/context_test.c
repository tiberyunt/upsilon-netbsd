/* Execute MD code against actual NetBSD LWP/CPU structures. BSD-2-Clause */
#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/cpu.h>
#include <sys/systm.h>
#include <machine/pcb.h>
#include <machine/context.h>
#include <machine/lock.h>
#include "boot.h"
extern struct lwp lwp0;
extern unsigned long ups_fault_unwinds;
extern struct lwp *ups_verify_switch(struct lwp *,struct lwp *,unsigned long);
static struct lwp workers[2];
static struct pcb worker_pcbs[2];
static unsigned char stacks[2][16384] __attribute__((aligned(16)));
static volatile unsigned visits[2];
static unsigned long private_pages[2][512] __attribute__((aligned(4096)));
#define PRIVATE_VA 0x8000000000UL
static unsigned long psr(void){unsigned long n;__asm__ volatile("MFSR PSR,%0":"=r"(n));return n;}
static unsigned long tp(void){unsigned long n;__asm__ volatile("MFTP %0":"=r"(n));return n;}
static void check(int condition,const char *why){if(!condition)ups_fail(why);}
static struct lwp *test_switch(struct lwp *old,struct lwp *next,unsigned long tag){
 old->l_stat=LSRUN;next->l_stat=LSONPROC;return ups_verify_switch(old,next,tag);
}
static void worker(void *arg){
 unsigned id=(unsigned)(uintptr_t)arg;struct lwp *me=&workers[id],*other=&workers[1-id];
 volatile unsigned long local=0x12340000UL+id;
 unsigned long threadp=0x11111000UL+id*0x1000;
 label_t guard;check(setjmp(&guard)==0,"unexpected worker onfault");me->l_md.md_onfault=&guard;
 for(unsigned i=0;i<20;i++){
  check(curlwp==me&&curcpu()->ci_onproc==me&&me->l_stat==LSONPROC,"switch current LWP");
  check(tp()==threadp,"switch thread pointer");
  unsigned long asid;__asm__ volatile("MFSR ASID,%0":"=r"(asid));
  unsigned long root;__asm__ volatile("MFSR PTBR0,%0":"=r"(root));
  check(root==worker_pcbs[id].pcb_ctx.sf_ptbr0,"switch page-table root");
  check(asid==17+id,"switch ASID");
  check(curcpu()->ci_cpl==((psr()>>8)&7),"switch software IPL");
  check(local==0x12340000UL+id,"switch local stack");
  unsigned long value;
  check(copyin((void *)PRIVATE_VA,&value,sizeof(value))==0&&value==0xabc00000UL+id*256+i,"per-LWP user mapping");
  value++;check(copyout(&value,(void *)PRIVATE_VA,sizeof(value))==0,"private user write");
  check(me->l_md.md_onfault==&guard,"per-LWP onfault isolation");
  visits[id]++;
  struct lwp *left=test_switch(me,id?&lwp0:other,0xfeed0000UL+id*256+i);
  check(left==(id?other:&lwp0),"switch return LWP or callee registers");
 }
 for(;;)check(test_switch(me,&lwp0,0xbaad0000UL+id)==&lwp0,"switch parked LWP");
}
void ups_context_tests(void){
 check(curlwp==&lwp0&&lwp_getpcb(curlwp)!=NULL,"boot LWP setup");
 __cpu_simple_lock_t lock;__cpu_simple_lock_init(&lock);
 check(__cpu_simple_lock_try(&lock)&&!__cpu_simple_lock_try(&lock),"native spinlock try");
 __cpu_simple_unlock(&lock);check(__SIMPLELOCK_UNLOCKED_P(&lock),"native spinlock release");
 int initial=curcpu()->ci_cpl;splx(IPL_SOFTBIO);
 int old=splhigh();check(old==IPL_SOFTBIO,"SPL raise result");check(splraise(IPL_VM)==IPL_HIGH&&curcpu()->ci_cpl==IPL_HIGH,"nested SPL raise");
 splx(IPL_SOFTBIO);check(((psr()>>8)&7)==IPL_SOFTBIO,"SPL lowering");splx(initial);
 check(cpu_lwp_setprivate(&lwp0,(void *)0x77777000)==0,"initial TP");
 check(cpu_lwp_setprivate(&lwp0,(void *)VM_MIN_KERNEL_ADDRESS)==EINVAL,"invalid TP rejection");
 check(ups_verify_switch(&lwp0,&lwp0,0xdead0000UL)==&lwp0,"self switch");
 label_t b;volatile unsigned jumped=0;
 if(!setjmp(&b)){jumped=1;__asm__ volatile("UAON":::"memory");longjmp(&b);}
 check(jumped==1&&tp()==0x77777000UL&&!(psr()&PSR_UA),"setjmp/longjmp");
 for(unsigned i=0;i<2;i++){
  workers[i].l_addr=&worker_pcbs[i];workers[i].l_lid=i+2;
  worker_pcbs[i].pcb_ctx.sf_tp=0xabcdef;
  check(ups_lwp_prepare(&workers[i],stacks[i],16,worker,NULL)==EINVAL&&worker_pcbs[i].pcb_ctx.sf_tp==0xabcdef,"reject undersized kernel stack");
  check(ups_lwp_prepare(&workers[i],stacks[i],sizeof(stacks[i]),worker,(void *)(uintptr_t)i)==0,"prepare kernel LWP");
  check(cpu_lwp_setprivate(&workers[i],(void *)(0x11111000UL+i*0x1000))==0,"prepare LWP TLS");
  worker_pcbs[i].pcb_ctx.sf_ptbr0=ups_pmap_clone_bootstrap();
  private_pages[i][0]=0xabc00000UL+i*256;
  ups_pmap_enter_root(worker_pcbs[i].pcb_ctx.sf_ptbr0,PRIVATE_VA,(unsigned long)private_pages[i],UPS_PTE_R|UPS_PTE_W|UPS_PTE_USER);
  worker_pcbs[i].pcb_ctx.sf_asid=17+i;
  worker_pcbs[i].pcb_ctx.sf_psr=PSR_FPD|((2UL+i*2)<<8);
 }
 unsigned long parent_root;__asm__ volatile("MFSR PTBR0,%0":"=r"(parent_root));
 unsigned long ticks=ups_ticks;
 for(unsigned i=0;i<20;i++){
  ups_compare(ups_cycles()+1); /* Deferred until the new stack restores its IPL. */
  check(test_switch(&lwp0,&workers[0],0xcafe0000UL+i)==&workers[1],"main switch return or callee registers");
  unsigned long root,asid;__asm__ volatile("MFSR PTBR0,%0\n\tMFSR ASID,%1":"=r"(root),"=r"(asid));
  check(root==parent_root&&asid==0,"main page-table root and ASID restore");
  check(curlwp==&lwp0&&tp()==0x77777000UL,"main context restore");
  check(((psr()>>8)&7)==IPL_HIGH,"main IPL restore");
 }
 unsigned long value;
 check(copyin((void *)PRIVATE_VA,&value,sizeof(value))==EFAULT,"parent mapping isolation");
 check(private_pages[0][0]==0xabc00000UL+20&&private_pages[1][0]==0xabc00100UL+20,"private mappings remain independent");
 check(ups_ticks==ticks+20,"interrupt delivery across context switch");
 check(curlwp->l_md.md_onfault==NULL,"main onfault isolation");
 check(visits[0]==20&&visits[1]==20,"kernel LWP counts");
 check(curcpu()->ci_intr_depth==0,"interrupt depth accounting");
 printf("context: 60 switches + 20 timer IRQs, registers, stacks, TLS, private mappings, roots/ASID, IPL passed\n");
}
void ups_copy_tests(void){
 char data[32],out[32];label_t outer;unsigned long unwinds=ups_fault_unwinds;
 memset(data,0,sizeof(data));memset(out,0x5a,sizeof(out));
 check(copyin((void *)0x40000000UL,data,4)==0,"copyin user text");
 check(copyout(out,(void *)0x40001000UL,sizeof(out))==0,"copyout user stack");
 check(copyin((void *)0x40001000UL,data,sizeof(data))==0&&memcmp(data,out,sizeof(data))==0,"copy round trip");
 check(copyout(out,(void *)0x40000000UL,1)==EFAULT,"copyout executable text");
 check(copyin((void *)0x200000UL,data,1)==EFAULT,"copyin kernel identity alias");
 check(copyin((void *)VM_MIN_KERNEL_ADDRESS,data,1)==EFAULT,"copyin high kernel pointer");
 check(copyout(out,(void *)(VM_MAXUSER_ADDRESS-1),2)==EFAULT,"copyout VA wrap");
 check(copyin(NULL,data,0)==0&&copyout(out,NULL,0)==0,"zero length copy");
 /* A valid user PTE pointing outside RAM passes permission checks then faults. */
 ups_pmap_enter(0x50000000UL,ups_memory+4096,UPS_PTE_R|UPS_PTE_W|UPS_PTE_USER);
 check(setjmp(&outer)==0,"unexpected outer fault handler");
 curlwp->l_md.md_onfault=&outer;
 check(copyin((void *)0x50000000UL,data,1)==EFAULT,"copyin bus fault recovery");
 check(curlwp->l_md.md_onfault==&outer&&!(psr()&PSR_UA),"copyin onfault and UA restore");
 check(copyout(out,(void *)0x50000000UL,1)==EFAULT,"copyout bus fault recovery");
 check(curlwp->l_md.md_onfault==&outer&&!(psr()&PSR_UA),"copyout onfault and UA restore");
 __asm__ volatile("UAON":::"memory");
 check(copyin((void *)0x50000000UL,data,1)==EFAULT&&(psr()&PSR_UA),"copy preserves prior UA on failure");
 check(copyin((void *)0x40000000UL,data,1)==0&&(psr()&PSR_UA),"copy preserves prior UA on success");
 __asm__ volatile("UAOFF":::"memory");
 check(ups_fault_unwinds==unwinds+3,"nested copy kernel-stack unwind");
 curlwp->l_md.md_onfault=NULL;
 printf("copy: user permissions, onfault stack unwind, EFAULT and UA restore passed\n");
 check(cpu_lwp_setprivate(&lwp0,(void *)0x40001000UL)==0,"prepare user TLS page");
}

void ups_user_tls_check(void){
 unsigned long n=0;
 check(copyin((void *)0x40001000UL,&n,sizeof(n))==0&&n==99,"user TP-relative storage");
 check(tp()==0x40001000UL&&!(psr()&PSR_UA),"user TLS and kernel UA state");
 printf("user TLS: MFTP and TP-relative memory passed\n");
}
