/* NetBSD CPU/LWP state and early kernel-thread creation. BSD-2-Clause */
#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <machine/pcb.h>
#include <machine/context.h>
struct cpu_info cpu_info_store[1];
unsigned long ups_fault_unwinds;
int ups_lwp_prepare(struct lwp *l,void *stack,size_t size,void (*entry)(void *),void *arg){
 if(!l||!l->l_addr||!stack||size<4096||!entry||(uintptr_t)stack>UINTPTR_MAX-size)return EINVAL;
 struct pcb *p=lwp_getpcb(l);
 uintptr_t top=((uintptr_t)stack+size)&~(uintptr_t)15;
 memset(p,0,sizeof(*p));
 /* Fake return from cpu_switchto leaves the trampoline at SP % 16 == 8. */
 p->pcb_ctx.sf_sp=top-16;
 *(uintptr_t *)(top-16)=(uintptr_t)ups_lwp_trampoline;
 p->pcb_ctx.sf_saved[0]=(uintptr_t)entry;p->pcb_ctx.sf_saved[1]=(uintptr_t)arg;
 p->pcb_ctx.sf_psr=PSR_FPD|((uint64_t)IPL_SCHED<<8);
 __asm__ volatile("MFSR PTBR0,%0\n\tMFSR ASID,%1":"=r"(p->pcb_ctx.sf_ptbr0),"=r"(p->pcb_ctx.sf_asid));
 l->l_cpu=curcpu();l->l_md.md_onfault=NULL;l->l_md.md_utf=NULL;l->l_md.md_astpending=0;
 l->l_stat=LSRUN;
 return 0;
}
int cpu_lwp_setprivate(struct lwp *l,void *address){
 if((uintptr_t)address>=VM_MAXUSER_ADDRESS)return EINVAL;
 struct pcb *p=lwp_getpcb(l);p->pcb_ctx.sf_tp=(uintptr_t)address;
 if(l==curlwp)__asm__ volatile("MTSR %0,TP"::"r"((uintptr_t)address):"memory");
 return 0;
}
int ups_fault_recover(struct trapframe *tf,unsigned long vector,unsigned long access){
 if((tf->tf_psr&PSR_MODE)!=0||(vector!=6&&vector!=7&&vector!=8&&vector!=13)||(access!=1&&access!=2))return 0;
 struct cpu_info *c=curcpu();if(!c||!c->ci_curlwp)return 0;
 label_t *f=c->ci_curlwp->l_md.md_onfault;if(!f)return 0;
 if(tf->tf_r[31]!=f->lb_sp)ups_fault_unwinds++;
 for(unsigned i=0;i<15;i++)tf->tf_r[16+i]=f->lb_reg[i];
 tf->tf_r[0]=EFAULT;tf->tf_r[31]=f->lb_sp;tf->tf_pc=f->lb_pc;tf->tf_psr=f->lb_psr;
 return 1;
}
static unsigned long read_psr(void){unsigned long n;__asm__ volatile("MFSR PSR,%0":"=r"(n));return n;}
int splraise(int level){
 unsigned long psr=read_psr();int old=(psr>>8)&7;
 if(level<0||level>IPL_HIGH)panic("splraise: invalid IPL");
 if(level>old){psr=(psr&~PSR_IPL)|((unsigned long)level<<8);__asm__ volatile("MTSR %0,PSR"::"r"(psr):"memory");}
 curcpu()->ci_cpl=level>old?level:old;
 return old;
}
void splx(int level){
 if(level<0||level>IPL_HIGH)panic("splx: invalid IPL");
 unsigned long psr=(read_psr()&~PSR_IPL)|((unsigned long)level<<8);
 __asm__ volatile("MTSR %0,PSR"::"r"(psr):"memory");curcpu()->ci_cpl=level;
}
int splhigh(void){return splraise(IPL_HIGH);}int splsched(void){return splraise(IPL_SCHED);}
int splvm(void){return splraise(IPL_VM);}int splsoftclock(void){return splraise(IPL_SOFTCLOCK);}
int splsoftbio(void){return splraise(IPL_SOFTBIO);}int splsoftnet(void){return splraise(IPL_SOFTNET);}
int splsoftserial(void){return splraise(IPL_SOFTSERIAL);}void spl0(void){splx(IPL_NONE);}

void ups_md_trap_enter(struct trapframe *tf,unsigned long vector){
 struct cpu_info *c=curcpu();if(!c)return;
 unsigned long psr=read_psr();c->ci_cpl=(psr>>8)&7;
 if((tf->tf_psr&PSR_MODE)==PSR_USER&&c->ci_curlwp)c->ci_curlwp->l_md.md_utf=tf;
 if(vector>=64)c->ci_intr_depth++;
}
void ups_md_trap_exit(const struct trapframe *tf,unsigned long vector){
 struct cpu_info *c=curcpu();if(!c)return;
 if(vector>=64)c->ci_intr_depth--;
 c->ci_cpl=(tf->tf_psr>>8)&7;
}

void ups_md_psr(unsigned long psr){
 struct cpu_info *c=curcpu();
 __asm__ volatile("MTSR %0,PSR"::"r"(psr):"memory");
 if(c)c->ci_cpl=(psr>>8)&7;
}
