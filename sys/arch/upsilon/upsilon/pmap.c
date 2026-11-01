/* Early single-CPU NetBSD pmap core. SPDX-License-Identifier: BSD-2-Clause
 * Fixed bootstrap pools are deliberately separate from UVM allocation. */
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <uvm/uvm_page.h>
#include <machine/pcb.h>
#include <machine/pte.h>
#define MAPS 8
#define TABLES 128
#define PVS 256
#define DBASE 0xffff800000000000UL
#define AD (UPS_PTE_A|UPS_PTE_D)
#define PERMS (UPS_PTE_R|UPS_PTE_W|UPS_PTE_X)
struct ups_pv_entry {
 pmap_t pm;
 vaddr_t va;
 paddr_t pa;
 struct vm_page *pg;
 struct ups_pv_entry *next,*pg_next;
 bool wired;
};
static struct pmap kernel_store,maps[MAPS];
struct pmap *const kernel_pmap_ptr=&kernel_store;
static uint64_t pt_pool[TABLES][512] __attribute__((aligned(4096)));
static pmap_t pt_owner[TABLES];
static struct ups_pv_entry pv_pool[PVS];
static paddr_t boot_root,boot_kernel_root;
static psize_t ram_size;
static struct vm_page *(*page_lookup)(paddr_t);
static bool ready;
static uint64_t *table(paddr_t pa){return (uint64_t *)(DBASE+pa);}
static void flush(void){__asm__ volatile("FENCE\n\tTLBI #0,#0":::"memory");}
static int pt_index(paddr_t pa){
 paddr_t base=(uintptr_t)pt_pool;
 if(pa<base||pa-base>=sizeof(pt_pool)||(pa&4095))return -1;
 return (int)((pa-base)>>12);
}
static paddr_t pt_alloc(pmap_t pm){
 for(unsigned i=0;i<TABLES;i++)if(!pt_owner[i]){
  pt_owner[i]=pm;memset(pt_pool[i],0,4096);return (uintptr_t)pt_pool[i];
 }
 return 0;
}
static void pt_free(paddr_t pa){int i=pt_index(pa);if(i<0||!pt_owner[i])panic("pmap: invalid table free");pt_owner[i]=NULL;}
static bool owned(paddr_t pa,pmap_t pm){int i=pt_index(pa);return i>=0&&pt_owner[i]==pm;}
static unsigned index_at(vaddr_t va,unsigned level){static const unsigned shifts[]={39,30,21,12};return (va>>shifts[level])&511;}
static uint64_t *lookup(pmap_t pm,vaddr_t va,unsigned *level){
 uint64_t *t=table(pm->pm_root);
 for(unsigned l=0;l<4;l++){
  uint64_t *p=&t[index_at(va,l)],v=*p;
  if(!(v&UPS_PTE_V))return NULL;
  if(l==3||(v&UPS_PTE_LARGE)){if(level)*level=l;return p;}
  t=table(v&UPS_PTE_PA);
 }
 return NULL;
}
/* Roll back intermediate allocations if PMAP_CANFAIL cannot build a path. */
static uint64_t *mutable_leaf(pmap_t pm,vaddr_t va){
 struct change {uint64_t *slot,old;paddr_t page;} changes[3];unsigned count=0;
 uint64_t *t=table(pm->pm_root);
 for(unsigned l=0;l<3;l++){
  uint64_t *slot=&t[index_at(va,l)],v=*slot;paddr_t pa=v&UPS_PTE_PA;
  if((v&UPS_PTE_V)&&(v&UPS_PTE_LARGE))panic("pmap: unexpected managed large page");
  if(!(v&UPS_PTE_V)||!owned(pa,pm)){
   paddr_t next=pt_alloc(pm);
   if(!next){
    while(count){struct change *c=&changes[--count];*c->slot=c->old;pt_free(c->page);}return NULL;
   }
   if(v&UPS_PTE_V)memcpy(table(next),table(pa),4096);
   changes[count++]=(struct change){slot,v,next};*slot=next|UPS_PTE_V;pa=next;
  }
  t=table(pa);
 }
 return &t[index_at(va,3)];
}
static void prune(pmap_t pm,vaddr_t va){
 paddr_t pages[4];uint64_t *slots[3];pages[0]=pm->pm_root;
 for(unsigned l=0;l<3;l++){
  slots[l]=&table(pages[l])[index_at(va,l)];uint64_t v=*slots[l];
  if(!(v&UPS_PTE_V)||(v&UPS_PTE_LARGE))return;pages[l+1]=v&UPS_PTE_PA;
 }
 for(int l=3;l>0;l--){
  if(!owned(pages[l],pm))return;
  uint64_t *t=table(pages[l]);for(unsigned i=0;i<512;i++)if(t[i]&UPS_PTE_V)return;
  *slots[l-1]=0;pt_free(pages[l]);
 }
}
static struct ups_pv_entry *find(pmap_t pm,vaddr_t va){
 for(struct ups_pv_entry *p=pm->pm_mappings;p;p=p->next)if(p->va==va)return p;
 return NULL;
}
static struct ups_pv_entry *pv_alloc(void){for(unsigned i=0;i<PVS;i++)if(!pv_pool[i].pm)return &pv_pool[i];return NULL;}
static void harvest(struct ups_pv_entry *p,uint64_t pte){if(p->pg)p->pg->mdpage.md_attrs|=pte&AD;}
static void pg_detach(struct ups_pv_entry *p){
 if(!p->pg)return;
 struct ups_pv_entry **at=&p->pg->mdpage.md_pv;while(*at&&*at!=p)at=&(*at)->pg_next;
 if(!*at)panic("pmap: missing reverse mapping");*at=p->pg_next;p->pg=NULL;p->pg_next=NULL;
}
static void pg_attach(struct ups_pv_entry *p,bool track){
 p->pg=track&&page_lookup?page_lookup(p->pa):NULL;
 if(p->pg){p->pg_next=p->pg->mdpage.md_pv;p->pg->mdpage.md_pv=p;}
}
static void unlink_mapping(struct ups_pv_entry *p){
 pmap_t pm=p->pm;uint64_t *pte=lookup(pm,p->va,NULL);if(!pte)panic("pmap: missing leaf");
 harvest(p,*pte);pg_detach(p);*pte=0;
 struct ups_pv_entry **at=&pm->pm_mappings;while(*at!=p)at=&(*at)->next;*at=p->next;
 pm->pm_stats.resident_count--;if(p->wired)pm->pm_stats.wired_count--;
 vaddr_t va=p->va;memset(p,0,sizeof(*p));prune(pm,va);
}
static uint64_t prot_bits(vm_prot_t prot){return (prot&VM_PROT_READ?UPS_PTE_R:0)|(prot&VM_PROT_WRITE?UPS_PTE_W:0)|(prot&VM_PROT_EXECUTE?UPS_PTE_X:0);}
static bool va_ok(pmap_t pm,vaddr_t va){
 return pm&&pm->pm_refs&&!(va&4095)&&(pm->pm_kernel?
  va>=VM_MIN_KERNEL_ADDRESS&&va<VM_MAX_KERNEL_ADDRESS:va>=4096&&va<VM_MAXUSER_ADDRESS);
}
static int enter(pmap_t pm,vaddr_t va,paddr_t pa,vm_prot_t prot,u_int flags,bool track){
 if(!va_ok(pm,va)||(pa&4095)||(pa&~UPS_PTE_PA)||(prot&~VM_PROT_ALL))return EINVAL;
 unsigned cache=flags&PMAP_CACHE_MASK;
 if(cache&&cache!=PMAP_NOCACHE&&cache!=PMAP_WRITE_COMBINE&&cache!=PMAP_WRITE_BACK&&cache!=PMAP_NOCACHE_OVR)return EINVAL;
 int s=splhigh();struct ups_pv_entry *p=find(pm,va);bool fresh=p==NULL;
 uint64_t *existing=lookup(pm,va,NULL),old=existing?*existing:0;
 /* Never replace the inherited bootstrap code, data or direct-map leaves. */
 if(existing&&!p){splx(s);return EBUSY;}
 if(!p){p=pv_alloc();if(!p)goto nomem;p->pm=pm;}
 uint64_t *pte=mutable_leaf(pm,va);
 if(!pte){if(fresh)memset(p,0,sizeof(*p));goto nomem;}
 uint64_t v=pa|UPS_PTE_V|prot_bits(prot)|(pm->pm_kernel?0:UPS_PTE_USER);
 if(cache==PMAP_NOCACHE||cache==PMAP_NOCACHE_OVR)v|=UPS_PTE_DEVICE;
 else if(cache==PMAP_WRITE_COMBINE)v|=1UL<<52;
 if(flags&VM_PROT_ALL)v|=UPS_PTE_A;if(flags&VM_PROT_WRITE)v|=UPS_PTE_D;
 if(!fresh){
  harvest(p,old);if(p->pa==pa)v|=old&AD;pg_detach(p);
  if(p->wired)pm->pm_stats.wired_count--;
 }else{p->next=pm->pm_mappings;pm->pm_mappings=p;pm->pm_stats.resident_count++;}
 p->va=va;p->pa=pa;p->wired=(flags&PMAP_WIRED)!=0;
 if(p->wired)pm->pm_stats.wired_count++;
 pg_attach(p,track);*pte=v;flush();splx(s);return 0;
nomem:
 splx(s);if(flags&PMAP_CANFAIL)return ENOMEM;panic("pmap: bootstrap resources exhausted");
}
void ups_pmap_init(paddr_t user,paddr_t kernel,psize_t ram,struct vm_page *(*lookup_page)(paddr_t)){
 if(ready)panic("pmap: bootstrap configured twice");boot_root=user;boot_kernel_root=kernel;ram_size=ram;page_lookup=lookup_page;pmap_init();
}
void pmap_init(void){
 if(ready)return;
 if(!boot_root||!boot_kernel_root)panic("pmap: missing bootstrap roots");
 kernel_store.pm_root=boot_kernel_root;kernel_store.pm_kernel=true;kernel_store.pm_refs=1;ready=true;
}
void ups_pmap_page_init(struct vm_page *pg){pg->mdpage.md_pv=NULL;pg->mdpage.md_attrs=0;}
pmap_t pmap_create(void){
 if(!ready)panic("pmap: create before initialization");
 int s=splhigh();
 for(unsigned i=0;i<MAPS;i++)if(!maps[i].pm_refs){
  pmap_t p=&maps[i];paddr_t root=pt_alloc(p);if(!root){splx(s);return NULL;}
  memset(p,0,sizeof(*p));p->pm_root=root;p->pm_refs=1;p->pm_asid=64+i;
  memcpy(table(root),table(boot_root),4096);flush();splx(s);return p;
 }
 splx(s);return NULL;
}
void pmap_reference(pmap_t p){int s=splhigh();if(!p||!p->pm_refs||p->pm_refs==UINT_MAX)panic("pmap: invalid reference");p->pm_refs++;splx(s);}
void pmap_destroy(pmap_t p){
 int s=splhigh();if(!p||p->pm_kernel||!p->pm_refs)panic("pmap: invalid destroy");
 if(p->pm_refs>1){p->pm_refs--;splx(s);return;}
 unsigned long current;__asm__ volatile("MFSR PTBR0,%0":"=r"(current));
 if((current&UPS_PTE_PA)==p->pm_root)panic("pmap: destroy active address space");
 while(p->pm_mappings)unlink_mapping(p->pm_mappings);
 for(unsigned i=0;i<TABLES;i++)if(pt_owner[i]==p)pt_owner[i]=NULL;
 memset(p,0,sizeof(*p));flush();splx(s);
}
int pmap_enter(pmap_t p,vaddr_t va,paddr_t pa,vm_prot_t prot,u_int flags){return enter(p,va,pa,prot,flags,true);}
bool pmap_extract(pmap_t p,vaddr_t va,paddr_t *out){
 if(!p||!p->pm_refs)return false;
 if(p->pm_kernel?(va<VM_MIN_KERNEL_ADDRESS||va>=VM_MAX_KERNEL_ADDRESS):va>=VM_MAXUSER_ADDRESS)return false;
 int s=splhigh();unsigned level;uint64_t *pte=lookup(p,va,&level);
 bool ok=pte&&(p->pm_kernel||(*pte&0x30)==UPS_PTE_USER);
 if(ok&&out){unsigned shift=level==1?30:level==2?21:12;*out=(*pte&UPS_PTE_PA)+(va&((1UL<<shift)-1));}
 splx(s);return ok;
}
static void range_check(pmap_t p,vaddr_t start,vaddr_t end){
 if(!p||!p->pm_refs||(start&4095)||(end&4095)||end<start||
    (p->pm_kernel?(start<VM_MIN_KERNEL_ADDRESS||end>VM_MAX_KERNEL_ADDRESS):end>VM_MAXUSER_ADDRESS))panic("pmap: invalid range");
}
void pmap_remove(pmap_t p,vaddr_t start,vaddr_t end){
 range_check(p,start,end);int s=splhigh();struct ups_pv_entry *v=p->pm_mappings;
 while(v){struct ups_pv_entry *next=v->next;if(v->va>=start&&v->va<end)unlink_mapping(v);v=next;}
 flush();splx(s);
}
static void restrict_mapping(struct ups_pv_entry *p,vm_prot_t prot){
 uint64_t *pte=lookup(p->pm,p->va,NULL),v=*pte,perms=v&prot_bits(prot);
 if(!perms){unlink_mapping(p);return;}*pte=(v&~PERMS)|perms;
}
void pmap_protect(pmap_t p,vaddr_t start,vaddr_t end,vm_prot_t prot){
 range_check(p,start,end);int s=splhigh();struct ups_pv_entry *v=p->pm_mappings;
 while(v){struct ups_pv_entry *next=v->next;if(v->va>=start&&v->va<end)restrict_mapping(v,prot);v=next;}
 flush();splx(s);
}
void pmap_unwire(pmap_t p,vaddr_t va){int s=splhigh();struct ups_pv_entry *v=find(p,va);if(!v)panic("pmap: unwire missing mapping");if(v->wired){v->wired=false;p->pm_stats.wired_count--;}splx(s);}
bool pmap_remove_all(pmap_t p){if(p->pm_kernel)return false;pmap_remove(p,0,VM_MAXUSER_ADDRESS);return true;}
void pmap_update(pmap_t p){(void)p;flush();}
void pmap_copy(pmap_t d,pmap_t s,vaddr_t da,vsize_t n,vaddr_t sa){(void)d;(void)s;(void)da;(void)n;(void)sa;/* MI permits this advisory operation to be a no-op. */}
void pmap_kenter_pa(vaddr_t va,paddr_t pa,vm_prot_t prot,u_int flags){if(enter(pmap_kernel(),va,pa,prot,flags|PMAP_WIRED,false))panic("pmap: invalid kenter");}
void pmap_kremove(vaddr_t va,vsize_t n){if(n>UINTPTR_MAX-va)panic("pmap: kremove overflow");pmap_remove(pmap_kernel(),va,va+n);}
static bool page_test(struct vm_page *pg,uint64_t bit,bool clear){
 int s=splhigh();bool found=(pg->mdpage.md_attrs&bit)!=0;
 for(struct ups_pv_entry *p=pg->mdpage.md_pv;p;p=p->pg_next){uint64_t *pte=lookup(p->pm,p->va,NULL);if(*pte&bit)found=true;if(clear)*pte&=~bit;}
 if(clear){pg->mdpage.md_attrs&=~bit;flush();}splx(s);return found;
}
bool pmap_is_referenced(struct vm_page *pg){return page_test(pg,UPS_PTE_A,false);}
bool pmap_is_modified(struct vm_page *pg){return page_test(pg,UPS_PTE_D,false);}
bool pmap_clear_reference(struct vm_page *pg){return page_test(pg,UPS_PTE_A,true);}
bool pmap_clear_modify(struct vm_page *pg){return page_test(pg,UPS_PTE_D,true);}
void pmap_page_protect(struct vm_page *pg,vm_prot_t prot){
 int s=splhigh();struct ups_pv_entry *p=pg->mdpage.md_pv;
 while(p){struct ups_pv_entry *next=p->pg_next;restrict_mapping(p,prot);p=next;}flush();splx(s);
}
void pmap_pv_protect(paddr_t pa,vm_prot_t prot){struct vm_page *pg=page_lookup?page_lookup(pa):NULL;if(pg)pmap_page_protect(pg,prot);}
static void ram_page(paddr_t pa){if((pa&4095)||pa>ram_size||4096>ram_size-pa)panic("pmap: invalid RAM page");}
void pmap_zero_page(paddr_t pa){ram_page(pa);memset(table(pa),0,4096);}
void pmap_copy_page(paddr_t src,paddr_t dst){ram_page(src);ram_page(dst);memmove(table(dst),table(src),4096);}
paddr_t pmap_phys_address(paddr_t n){return n<<12;}
void pmap_virtual_space(vaddr_t *start,vaddr_t *end){*start=VM_MIN_KERNEL_ADDRESS+0x10000000000UL;*end=VM_MAX_KERNEL_ADDRESS;}
void pmap_activate(struct lwp *l){
 pmap_t p=l->l_proc&&l->l_proc->p_vmspace?l->l_proc->p_vmspace->vm_map.pmap:pmap_kernel();
 struct pcb *pcb=lwp_getpcb(l);int s=splhigh();
 pcb->pcb_ctx.sf_ptbr0=p->pm_kernel?boot_root:p->pm_root;pcb->pcb_ctx.sf_asid=p->pm_asid;
 if(l==curlwp)__asm__ volatile("MTSR %0,PTBR0\n\tMTSR %1,ASID"::"r"(pcb->pcb_ctx.sf_ptbr0),"r"(pcb->pcb_ctx.sf_asid):"memory");
 splx(s);
}
void pmap_deactivate(struct lwp *l){(void)l;/* UP: leave the root until activate/switchto replaces it. */}
unsigned int ups_pmap_table_usage(void){unsigned n=0;for(unsigned i=0;i<TABLES;i++)if(pt_owner[i])n++;return n;}
unsigned int ups_pmap_mapping_usage(void){unsigned n=0;for(unsigned i=0;i<PVS;i++)if(pv_pool[i].pm)n++;return n;}
unsigned long ups_pmap_query(pmap_t pm,vaddr_t va){uint64_t *p=lookup(pm,va,NULL);return p?*p:0;}
