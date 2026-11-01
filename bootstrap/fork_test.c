/* Probe-only callers of NetBSD MD fork/exec hooks. BSD-2-Clause */
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/exec.h>
#include <uvm/uvm_page.h>
#include <machine/pcb.h>
#include <machine/context.h>
#include "boot.h"
#define TEXT 0x70000000UL
#define STACK 0x70001000UL
#define TOP (STACK+4096)
static struct lwp child;
static struct proc process;
static struct vmspace space;
static unsigned char area[USPACE+32] __attribute__((aligned(16)));
static unsigned long user_stack[512] __attribute__((aligned(4096)));
static struct trapframe parent_frame;
static struct lwp *parent;
static unsigned starts,entries,exits,mode;
int ups_test_returning_kthread;
extern char ups_fork_user_start[],ups_fork_user_exec[],ups_fork_user_end[];
extern struct lwp *ups_verify_switch(struct lwp *,struct lwp *,unsigned long);
static uintptr_t base(void){return (uintptr_t)area+16;}
static void check(bool ok,const char *why){if(!ok)ups_fail(why);}
static unsigned long tp(void){unsigned long n;__asm__ volatile("MFTP %0":"=r"(n));return n;}
/* A full kernel supplies this from kern_lwp.c. This harness checks the native
 * trampoline's calling convention without pretending to run the scheduler. */
void lwp_startup(struct lwp *old,struct lwp *new){
 unsigned long local;
 check(old==parent&&new==&child&&curlwp==new,"fork MI startup arguments");
 check((uintptr_t)&local>base()+sizeof(struct pcb)&&(uintptr_t)&local<base()+USPACE,"fork own kernel stack");
 check(curcpu()->ci_cpl==IPL_SCHED,"fork startup IPL");
 starts++;spl0();
}
static void exited(unsigned long status) __attribute__((noreturn));
static void child_entry(void *arg){
 check(arg==&child&&starts==entries+1,"fork entry argument/order");entries++;
 check(child.l_md.md_onfault==NULL&&!child.l_md.md_astpending,"fork clean MD state");
 check(tp()==(mode==1?0:STACK+32),"fork/exec live TP");
 if(mode==2){if(ups_test_returning_kthread)return;exited(0);}
 if(!mode)md_child_return(&child);
 /* Returning here exercises the native trapframe-to-user trampoline. */
}
static void exited(unsigned long status){
 check(status==0&&curlwp==&child,"fork/exec user program");
 unsigned long root,asid;__asm__ volatile("MFSR PTBR0,%0\n\tMFSR ASID,%1":"=r"(root),"=r"(asid));
 check(root==space.vm_map.pmap->pm_root&&asid==space.vm_map.pmap->pm_asid,"fork active pmap");
 if(!mode)check(user_stack[4]==123,"fork inherited user TP access");
 exits++;child.l_stat=LSRUN;parent->l_stat=LSONPROC;
 (void)ups_verify_switch(&child,parent,0xf0120000UL+mode);
 ups_fail("completed fork test resumed");
}
static void user_exit(struct trapframe *tf){
 check(curlwp==&child&&tf==child.l_md.md_utf,"fork syscall kernel stack/trapframe");
 /* Probe exit path only: continue on the child stack, then switch to parent.
  * MI exit/exit2, process reclamation and scheduler teardown remain ahead. */
 tf->tf_r[0]=tf->tf_r[1];tf->tf_pc=(uintptr_t)exited;
 tf->tf_r[31]=base()+USPACE-1032;tf->tf_psr=PSR_FPD|PSR_IPL;
}
void ups_fork_tests(void){
 parent=curlwp;struct trapframe *saved_tf=parent->l_md.md_utf;
 struct pcb *parent_pcb=lwp_getpcb(parent);
 unsigned long saved_tp=tp(),saved_fp=parent_pcb->pcb_fpregs[3];
 check((uintptr_t)(ups_fork_user_end-ups_fork_user_start)<=4096,"fork user program size");
 for(mode=0;mode<3;mode++){
  memset(area,0xa5,sizeof(area));memset(&child,0,sizeof(child));memset(user_stack,0,sizeof(user_stack));
  memset(&parent_frame,0,sizeof(parent_frame));
  for(unsigned i=0;i<32;i++)parent_frame.tf_r[i]=0x55000000UL+i;
  parent_frame.tf_r[20]=0x1122334455667788UL;
  parent_frame.tf_pc=TEXT;parent_frame.tf_r[31]=TOP;
  parent_frame.tf_psr=PSR_USER|PSR_UA|PSR_IPL|15;
  parent->l_md.md_utf=mode==2?NULL:&parent_frame;parent_pcb->pcb_fpregs[3]=0x987654321UL;
  check(!cpu_lwp_setprivate(parent,(void *)(STACK+32)),"fork parent TP setup");
  child.l_addr=(void *)base();child.l_cpu=curcpu();child.l_stat=LSRUN;
  process.p_vmspace=&space;process.p_psstrp=STACK+128;child.l_proc=&process;
  pmap_t pm=pmap_create();check(pm!=NULL,"fork pmap create");space.vm_map.pmap=pm;
  check(!pmap_enter(pm,TEXT,(uintptr_t)ups_fork_user_start,VM_PROT_READ|VM_PROT_EXECUTE,PMAP_CANFAIL),"fork text mapping");
  check(!pmap_enter(pm,STACK,(uintptr_t)user_stack,VM_PROT_READ|VM_PROT_WRITE,PMAP_CANFAIL),"fork stack mapping");
  cpu_lwp_fork(parent,&child,mode?NULL:(void *)STACK,4096-32,child_entry,&child);
  struct pcb *pcb=lwp_getpcb(&child);struct trapframe *tf=child.l_md.md_utf;
  check(tf==(struct trapframe *)(base()+USPACE)-1&&cpu_lwp_pc(&child)==(mode==2?0:TEXT),"fork trapframe placement/PC");
  check(tf->tf_r[20]==(mode==2?0:parent_frame.tf_r[20])&&pcb->pcb_fpregs[3]==0x987654321UL,"fork inherited register state");
  check(tf->tf_r[31]==(mode==2?0:mode==1?TOP:TOP-32)&&!(tf->tf_psr&(PSR_IPL|PSR_UA))&&(tf->tf_psr&PSR_MODE)==PSR_USER,"fork stack and privilege sanitization");
  check(pcb->pcb_ctx.sf_ptbr0==pm->pm_root&&pcb->pcb_ctx.sf_asid==pm->pm_asid,"fork PCB address space");
  if(mode==1){
   struct exec_package pack;memset(&pack,0,sizeof(pack));
   pack.ep_entry=TEXT+(ups_fork_user_exec-ups_fork_user_start);
   setregs(&child,&pack,TOP-1);
   check(tp()==STACK+32&&pcb->pcb_ctx.sf_tp==0,"inactive exec TP reset");
   check(!pcb->pcb_fpregs[3]&&!pcb->pcb_fpsr&&tf->tf_r[0]==0&&tf->tf_r[1]==process.p_psstrp&&tf->tf_r[20]==0,"exec clean registers");
   check(tf->tf_psr==(PSR_USER|PSR_FPD)&&tf->tf_r[31]==TOP-16,"exec initial PSR/aligned SP");
  }
  if(mode<2)user_stack[(tf->tf_r[31]-STACK)/8]=42;
  ups_user_exit_hook=user_exit;parent->l_stat=LSRUN;child.l_stat=LSONPROC;
  check(ups_verify_switch(parent,&child,0xca110000UL+mode)==&child,"fork parent resume/callee registers");
  ups_user_exit_hook=NULL;
  check(starts==mode+1&&entries==mode+1&&exits==mode+1,"fork execution counts");
  for(unsigned i=0;i<16;i++)check(area[i]==0xa5&&area[USPACE+16+i]==0xa5,"fork uarea boundary guards");
  check(tp()==STACK+32&&parent_frame.tf_r[20]==0x1122334455667788UL,"fork parent state preserved");
  pmap_destroy(pm);
 }
 parent->l_md.md_utf=saved_tf;parent_pcb->pcb_fpregs[3]=saved_fp;
 check(!cpu_lwp_setprivate(parent,(void *)saved_tp),"fork parent TP restore");
 check(!ups_pmap_table_usage()&&!ups_pmap_mapping_usage(),"fork pmap reclamation");
 printf("fork/exec: MI startup ABI, kthread/child stacks, inherited state, setregs and native user return passed\n");
}
