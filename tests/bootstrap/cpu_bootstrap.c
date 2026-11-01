/* Probe stand-in for MI lwp0 initialization. BSD-2-Clause */
#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <machine/pcb.h>
#include <machine/context.h>
struct lwp lwp0;
static struct pcb pcb0;
void ups_cpu_init(void){
 struct cpu_info *c=&cpu_info_store[0];
 memset(c,0,sizeof(*c));memset(&lwp0,0,sizeof(lwp0));
 lwp0.l_addr=&pcb0;lwp0.l_cpu=c;lwp0.l_lid=1;lwp0.l_stat=LSONPROC;
 c->ci_curlwp=c->ci_onproc=&lwp0;c->ci_cpl=IPL_HIGH;
 __asm__ volatile("MTSR %0,SCRATCH0"::"r"(c):"memory");
}
