/* Execute NetBSD pmap interfaces against the actual Upsilon MMU. BSD-2-Clause */
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <uvm/uvm_page.h>
#include <machine/pcb.h>
#include <machine/pte.h>
#include "boot.h"
#define UVA 0x10000000000UL
#define RW (VM_PROT_READ|VM_PROT_WRITE)
#define KVA (VM_MIN_KERNEL_ADDRESS+0x10000000000UL)
static unsigned long frames[3][512] __attribute__((aligned(4096)));
static struct vm_page pages[3];
static struct proc test_proc;
static struct vmspace test_space;
static void check(int condition,const char *what){if(!condition)ups_fail(what);}
static paddr_t physical(unsigned i){return (uintptr_t)frames[i];}
static struct vm_page *lookup_page(paddr_t pa){
 for(unsigned i=0;i<3;i++)if(pa==(pages[i].phys_addr&~4095UL))return &pages[i];return NULL;
}
static void activate(pmap_t pm){
 test_proc.p_vmspace=&test_space;test_space.vm_map.pmap=pm;curlwp->l_proc=&test_proc;pmap_activate(curlwp);
 unsigned long root,asid;__asm__ volatile("MFSR PTBR0,%0\n\tMFSR ASID,%1":"=r"(root),"=r"(asid));
 check(root==((struct pcb *)lwp_getpcb(curlwp))->pcb_ctx.sf_ptbr0&&asid==pm->pm_asid,"pmap activate registers/PCB");
}
static int raw_store(vaddr_t va,unsigned long value){
 label_t f;label_t *saved=curlwp->l_md.md_onfault;int error=setjmp(&f);
 if(error){curlwp->l_md.md_onfault=saved;return error;}
 curlwp->l_md.md_onfault=&f;__asm__ volatile("UAON":::"memory");
 __asm__ volatile("MOV.q %1,(%0)"::"r"(va),"r"(value):"memory");
 __asm__ volatile("MTSR %0,PSR"::"r"(f.lb_psr):"memory");curlwp->l_md.md_onfault=saved;return 0;
}
static void resource_tests(void){
 pmap_t a=pmap_create(),b=pmap_create(),c=pmap_create();check(a&&b&&c,"pmap resource maps");
 unsigned base=ups_pmap_table_usage(),count=0;
 for(unsigned i=1;i<200;i++){
  int error=pmap_enter(a,(vaddr_t)i<<39,physical(0),RW,PMAP_CANFAIL);
  if(error){check(error==ENOMEM,"page-table exhaustion errno");break;}count++;
 }
 check(count>0&&count<199&&pmap_resident_count(a)==count,"page-table exhaustion bounded");
 unsigned used=ups_pmap_table_usage(),mappings=ups_pmap_mapping_usage();
 for(unsigned i=0;i<5;i++){
  check(pmap_enter(a,(vaddr_t)(count+1)<<39,physical(0),RW,PMAP_CANFAIL)==ENOMEM,"repeat table allocation failure");
  check(ups_pmap_table_usage()==used&&ups_pmap_mapping_usage()==mappings,"table failure rollback");
 }
 check(!pmap_extract(a,(vaddr_t)(count+1)<<39,NULL),"failed mapping not published");
 check(pmap_remove_all(a)&&ups_pmap_table_usage()==base,"page-table reclamation");
 pmap_destroy(a);pmap_destroy(b);pmap_destroy(c);
 check(ups_pmap_table_usage()==0&&ups_pmap_mapping_usage()==0,"pmap destruction cleanup");
 a=pmap_create();check(a!=NULL,"pmap metadata map");count=0;
 for(unsigned i=0;i<400;i++){
  int error=pmap_enter(a,UVA+(vaddr_t)i*4096,physical(0),RW,PMAP_CANFAIL);
  if(error){check(error==ENOMEM,"mapping metadata exhaustion errno");break;}count++;
 }
 check(count>0&&count<400&&ups_pmap_mapping_usage()==count,"mapping metadata exhaustion bounded");
 used=ups_pmap_table_usage();
 check(pmap_enter(a,UVA+(vaddr_t)count*4096,physical(0),RW,PMAP_CANFAIL)==ENOMEM,"metadata allocation failure");
 check(ups_pmap_table_usage()==used&&pmap_resident_count(a)==count,"metadata failure rollback");
 check(pmap_enter(a,UVA,physical(1),RW,PMAP_CANFAIL)==0,"replace while metadata full");
 paddr_t pa;check(pmap_extract(a,UVA,&pa)&&pa==physical(1),"full-pool replacement target");
 pmap_destroy(a);check(!ups_pmap_table_usage()&&!ups_pmap_mapping_usage(),"metadata pool reclamation");
 pmap_t held[16];unsigned n=0;
 while(n<16&&(held[n]=pmap_create())!=NULL)n++;
 check(n>1&&n<16,"pmap-object exhaustion bounded");
 for(unsigned i=0;i<n;i++)pmap_destroy(held[i]);
 a=pmap_create();check(a&&a->pm_stats.resident_count==0&&!pmap_extract(a,UVA,NULL),"fresh ASID/root reuse");pmap_destroy(a);
 printf("pmap resources: CANFAIL rollback, pool exhaustion, reclamation and ASID reuse passed\n");
}
void ups_pmap_tests(void){
 paddr_t root0,root1;__asm__ volatile("MFSR PTBR0,%0\n\tMFSR PTBR1,%1":"=r"(root0),"=r"(root1));
 struct proc *saved_proc=curlwp->l_proc;
 for(unsigned i=0;i<3;i++){pages[i].phys_addr=physical(i)|5;ups_pmap_page_init(&pages[i]);frames[i][0]=0xabc000+i;}
 ups_pmap_init(root0,root1,ups_memory,lookup_page);
 pmap_t a=pmap_create(),b=pmap_create();check(a&&b&&a->pm_asid!=b->pm_asid,"pmap create/ASID");
 pmap_reference(a);pmap_destroy(a);check(a->pm_refs==1,"pmap reference accounting");
 check(!pmap_enter(a,UVA,physical(0),RW,PMAP_WIRED|PMAP_CANFAIL),"pmap enter wired");
 check(!pmap_enter(a,UVA+4096,physical(0),RW,PMAP_CANFAIL),"pmap alias enter");
 check(!pmap_enter(b,UVA,physical(1),RW,PMAP_CANFAIL),"second pmap enter");
 check(pmap_resident_count(a)==2&&pmap_wired_count(a)==1,"resident/wired counts");
 paddr_t pa;check(pmap_extract(a,UVA+27,&pa)&&pa==physical(0)+27,"pmap extract offset");
 check(!pmap_is_referenced(&pages[0])&&!pmap_is_modified(&pages[0]),"initial access bits");
 activate(a);unsigned long value=0x12345678,result=0;
 check(!copyout(&value,(void *)UVA,sizeof(value))&&!copyin((void *)(UVA+4096),&result,sizeof(result))&&result==value,"live pmap alias data");
 check(pmap_is_modified(&pages[0])&&pmap_is_referenced(&pages[0]),"hardware access/dirty bits");
 activate(b);check(!copyin((void *)UVA,&result,sizeof(result))&&result==0xabc001,"live address-space isolation");
 activate(a);check(!copyin((void *)UVA,&result,sizeof(result))&&result==value,"pmap activation restore");
 check(pmap_clear_reference(&pages[0])&&pmap_clear_modify(&pages[0]),"clear access/dirty bits");
 check(!pmap_is_referenced(&pages[0])&&!pmap_is_modified(&pages[0]),"access/dirty bits cleared across aliases");
 check(!copyin((void *)(UVA+4096),&result,sizeof(result))&&pmap_is_referenced(&pages[0])&&!pmap_is_modified(&pages[0]),"read re-references without dirtying");
 pmap_protect(a,UVA,UVA+4096,VM_PROT_READ);pmap_update(a);
 check(copyout(&value,(void *)UVA,sizeof(value))==EFAULT&&raw_store(UVA,value)==EFAULT,"read-only mapping hardware fault");
 check(!copyout(&value,(void *)(UVA+4096),sizeof(value)),"range protection leaves alias writable");
 pmap_protect(a,UVA,UVA+4096,RW);check(copyout(&value,(void *)UVA,sizeof(value))==EFAULT,"protect cannot grant write");
 check(!pmap_enter(a,UVA,physical(0),RW,PMAP_WIRED|PMAP_CANFAIL),"explicit mapping promotion");
 check(pmap_resident_count(a)==2&&pmap_wired_count(a)==1,"same-PA replacement counts");
 pmap_unwire(a,UVA);pmap_unwire(a,UVA);check(pmap_wired_count(a)==0,"unwire idempotence");
 pmap_page_protect(&pages[0],VM_PROT_READ);
 check(copyout(&value,(void *)(UVA+4096),sizeof(value))==EFAULT,"reverse-map protection");
 pmap_pv_protect(physical(0),VM_PROT_NONE);
 check(!pmap_extract(a,UVA,NULL)&&!pmap_extract(a,UVA+4096,NULL)&&pmap_resident_count(a)==0,"reverse-map removal");
 check(pmap_is_modified(&pages[0])&&pages[0].mdpage.md_pv==NULL,"removed mapping retains dirty history");
 check(pmap_clear_modify(&pages[0])&&!pmap_is_modified(&pages[0]),"clear saved dirty history");
 check(!pmap_enter(a,UVA,physical(0),RW,PMAP_CANFAIL),"replacement setup");
 check(!pmap_enter(a,UVA,physical(2),RW,PMAP_CANFAIL|VM_PROT_READ),"different-PA replacement");
 pmap_page_protect(&pages[0],VM_PROT_NONE);
 check(pmap_extract(a,UVA,&pa)&&pa==physical(2)&&pmap_is_referenced(&pages[2]),"replacement PV and access seeding");
 pmap_kenter_pa(KVA,physical(2),RW,PMAP_NOCACHE);pmap_update(pmap_kernel());
 check(((ups_pmap_query(pmap_kernel(),KVA)>>52)&3)==2,"kernel device memory type");
 *(volatile unsigned long *)KVA=0xdeadbeef;
 pmap_page_protect(&pages[2],VM_PROT_NONE);
 check(!pmap_extract(a,UVA,NULL)&&*(volatile unsigned long *)KVA==0xdeadbeef,"unmanaged kenter excludes PV protection");
 check(pmap_wired_count(pmap_kernel())==1&&pmap_extract(pmap_kernel(),KVA+8,&pa)&&pa==physical(2)+8,"kernel wired map/extract");
 pmap_kremove(KVA,4096);check(!pmap_extract(pmap_kernel(),KVA,NULL)&&pmap_wired_count(pmap_kernel())==0,"kernel mapping removal");
 check(pmap_extract(pmap_kernel(),UPS_KERNBASE+physical(2)+27,&pa)&&pa==physical(2)+27,"large bootstrap leaf extraction");
 pmap_zero_page(physical(2));for(unsigned i=0;i<512;i++)check(!frames[2][i],"zero physical page");
 frames[0][7]=0xbadc0ffeeUL;pmap_copy_page(physical(0),physical(2));check(!memcmp(frames[0],frames[2],4096),"copy physical page");
 check(pmap_enter(a,UVA+1,physical(0),RW,PMAP_CANFAIL)==EINVAL,"unaligned VA rejection");
 check(pmap_enter(a,UVA,physical(0)+1,RW,PMAP_CANFAIL)==EINVAL,"unaligned PA rejection");
 check(pmap_enter(a,VM_MAXUSER_ADDRESS,physical(0),RW,PMAP_CANFAIL)==EINVAL,"kernel VA rejection");
 check(pmap_enter(a,UVA,1UL<<53,RW,PMAP_CANFAIL)==EINVAL,"physical width rejection");
 check(pmap_enter(a,0x200000,physical(0),RW,PMAP_CANFAIL)==EBUSY,"bootstrap identity mapping preservation");
 /* L0[0] shares the bootstrap's lower tables: modifying a must clone the
  * path without publishing the mapping in b or changing the running code. */
 const vaddr_t low=0x40000000UL;
 check(!pmap_enter(a,low,physical(0),RW,PMAP_CANFAIL),"inherited table path clone");
 check(!pmap_extract(b,low,NULL)&&pmap_extract(a,low,&pa)&&pa==physical(0),"cloned path isolation");
 check(!copyin((void *)low,&result,sizeof(result))&&result==frames[0][0],"cloned path live access");
 check(!pmap_enter(a,low+4096,physical(1),RW,PMAP_CANFAIL)&&
       !pmap_enter(a,low+8192,physical(2),RW,PMAP_CANFAIL),"range removal setup");
 pmap_remove(a,low+4096,low+8192);pmap_update(a);
 check(pmap_extract(a,low,NULL)&&!pmap_extract(a,low+4096,NULL)&&
       pmap_extract(a,low+8192,NULL),"range removal preserves boundary neighbors");
 check(copyin((void *)(low+4096),&result,sizeof(result))==EFAULT,"removed mapping copy fault");
 pmap_protect(a,low+8192,low+12288,VM_PROT_NONE);
 check(!pmap_extract(a,low+8192,NULL),"no-access protection removes mapping");
 pmap_deactivate(curlwp);activate(pmap_kernel());curlwp->l_proc=saved_proc;
 check(pmap_remove_all(a)&&pmap_remove_all(b),"remove user mappings");pmap_destroy(a);pmap_destroy(b);
 check(!ups_pmap_table_usage()&&!ups_pmap_mapping_usage(),"pmap lifecycle cleanup");
 printf("pmap: lifecycle, activation, isolation, PV/protection, ref/dirty, kernel maps passed\n");
 resource_tests();
}
