/* NetBSD LWP fork and exec register setup. SPDX-License-Identifier: BSD-2-Clause */
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/exec.h>
#include <machine/context.h>
#include <machine/pcb.h>
#include <machine/pmap.h>

void cpu_lwp_fork(struct lwp *parent,struct lwp *child,void *stack,size_t size,
    void (*func)(void *),void *arg){
 if(!parent||!child||parent==child||!parent->l_addr||!child->l_addr||!func||
    (parent!=curlwp&&parent!=&lwp0))panic("cpu_lwp_fork: invalid LWP");
 uintptr_t base=(uintptr_t)lwp_getpcb(child)-UAREA_PCB_OFFSET;
 if((base&15)||base>UINTPTR_MAX-USPACE)panic("cpu_lwp_fork: invalid uarea");
 struct pcb *pcb=lwp_getpcb(child);
 *pcb=*(struct pcb *)lwp_getpcb(parent);
 struct trapframe *tf=(struct trapframe *)(base+USPACE)-1;
 if(parent->l_md.md_utf)*tf=*parent->l_md.md_utf;
 else memset(tf,0,sizeof(*tf));
 if(stack){
  uintptr_t lo=(uintptr_t)stack;
  if(lo<PAGE_SIZE||lo>=VM_MAXUSER_ADDRESS||size>VM_MAXUSER_ADDRESS-lo)
   panic("cpu_lwp_fork: invalid user stack");
  tf->tf_r[31]=(lo+size)&~15UL;
 }
 /* No kernel privilege, UA, IPL or FP enable may reach the child. */
 tf->tf_psr=(tf->tf_psr&15)|PSR_USER|PSR_FPD;
 memset(&pcb->pcb_ctx,0,sizeof(pcb->pcb_ctx));
 pcb->pcb_ctx.sf_sp=(uintptr_t)tf-16;
 *(uintptr_t *)pcb->pcb_ctx.sf_sp=(uintptr_t)ups_fork_trampoline;
 pcb->pcb_ctx.sf_saved[0]=(uintptr_t)func;
 pcb->pcb_ctx.sf_saved[1]=(uintptr_t)arg;
 pcb->pcb_ctx.sf_saved[2]=(uintptr_t)child;
 pcb->pcb_ctx.sf_psr=PSR_FPD|((uint64_t)IPL_SCHED<<8);
 pcb->pcb_ctx.sf_tp=((struct pcb *)lwp_getpcb(parent))->pcb_ctx.sf_tp;
 if(parent==curlwp)__asm__ volatile("MFSR TP,%0":"=r"(pcb->pcb_ctx.sf_tp));
 child->l_md.md_utf=tf;child->l_md.md_onfault=NULL;child->l_md.md_astpending=0;
 pmap_activate(child);
}
void md_child_return(struct lwp *l){
 struct trapframe *tf=l->l_md.md_utf;
 tf->tf_r[0]=0;tf->tf_r[1]=1;
 tf->tf_psr=(tf->tf_psr&14)|PSR_USER|PSR_FPD;
}
vaddr_t cpu_lwp_pc(struct lwp *l){return l->l_md.md_utf?l->l_md.md_utf->tf_pc:0;}
void setregs(struct lwp *l,struct exec_package *pack,vaddr_t stack){
 if(!l->l_md.md_utf||pack->ep_entry<PAGE_SIZE||pack->ep_entry>=VM_MAXUSER_ADDRESS||
    stack<PAGE_SIZE||stack>=VM_MAXUSER_ADDRESS)panic("setregs: invalid user context");
 struct trapframe *tf=l->l_md.md_utf;struct pcb *pcb=lwp_getpcb(l);
 memset(tf,0,sizeof(*tf));
 tf->tf_pc=pack->ep_entry;tf->tf_r[31]=stack&~15UL;
 /* Initial crt0(cleanup, ps_strings); argc/argv remain on the user stack. */
 tf->tf_r[1]=l->l_proc?l->l_proc->p_psstrp:0;
 tf->tf_psr=PSR_USER|PSR_FPD;
 memset(pcb->pcb_fpregs,0,sizeof(pcb->pcb_fpregs));pcb->pcb_fpsr=0;
 l->l_md.md_onfault=NULL;l->l_md.md_astpending=0;
 (void)cpu_lwp_setprivate(l,NULL);
}

/* A kernel-thread entry must not fall through into an empty user frame. */
void ups_user_resume(struct trapframe *) __attribute__((noreturn));
void ups_return_user(struct trapframe *tf){
 if(!tf||tf->tf_pc<PAGE_SIZE||tf->tf_pc>=VM_MAXUSER_ADDRESS||
    tf->tf_r[31]<PAGE_SIZE||tf->tf_r[31]>=VM_MAXUSER_ADDRESS||
    (tf->tf_r[31]&15)||(tf->tf_psr&~15UL)!=(PSR_USER|PSR_FPD))
  panic("ups_return_user: invalid user context or returned kernel thread");
 ups_user_resume(tf);
}
