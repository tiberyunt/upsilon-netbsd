/* Early page tables for the Upsilon NetBSD bring-up probe.
 * This is not the UVM pmap implementation. SPDX-License-Identifier: BSD-2-Clause */
#include "boot.h"
#define NPAGES 32
static unsigned long tables[NPAGES][512] __attribute__((aligned(4096)));
static unsigned used;
static unsigned long *root0,*root1;
static unsigned long *alloc_table(void){
 if(used==NPAGES)ups_fail("bootstrap page-table pool exhausted");
 return tables[used++];
}
static unsigned long *descend(unsigned long *tab,unsigned index,unsigned level){
 unsigned long old=tab[index];
 if(!(old&UPS_PTE_V)){unsigned long *next=alloc_table();tab[index]=(unsigned long)next|UPS_PTE_V;return next;}
 if(old&UPS_PTE_LARGE){
  if(level!=2)ups_fail("unexpected large-page level");
  unsigned long *next=alloc_table(),base=old&UPS_PTE_PA,flags=old&~UPS_PTE_PA&~UPS_PTE_LARGE;
  for(unsigned i=0;i<512;i++)next[i]=(base+i*4096)|flags;
  tab[index]=(unsigned long)next|UPS_PTE_V;return next;
 }
 return (unsigned long *)(old&UPS_PTE_PA);
}
void ups_pmap_enter_root(unsigned long root,unsigned long va,unsigned long pa,unsigned long flags){
 unsigned long *t=(unsigned long *)root;
 t=descend(t,(va>>39)&511,0);t=descend(t,(va>>30)&511,1);t=descend(t,(va>>21)&511,2);
 t[(va>>12)&511]=(pa&UPS_PTE_PA)|flags|UPS_PTE_V;
 __asm__ volatile("TLBI #0,#0":::"memory");
}
void ups_pmap_enter(unsigned long va,unsigned long pa,unsigned long flags){
 ups_pmap_enter_root((unsigned long)(va>>47?root1:root0),va,pa,flags);
}
static void map_ram(unsigned long va,unsigned long memory){
 for(unsigned long pa=0;pa<memory;){
  unsigned long *t=va>>47?root1:root0;
  t=descend(t,(va>>39)&511,0);t=descend(t,(va>>30)&511,1);
  if(!(pa&0x1fffff)&&memory-pa>=0x200000){t[(va>>21)&511]=pa|UPS_PTE_V|UPS_PTE_R|UPS_PTE_W|UPS_PTE_X|UPS_PTE_LARGE;pa+=0x200000;va+=0x200000;}
  else {ups_pmap_enter(va,pa,UPS_PTE_R|UPS_PTE_W|UPS_PTE_X);pa+=4096;va+=4096;}
 }
}
void ups_pmap_bootstrap(unsigned long memory){
 root0=alloc_table();root1=alloc_table();
 map_ram(0,memory);map_ram(UPS_KERNBASE,memory);
 ups_pmap_enter(0xffff0000UL,0xffff0000UL,UPS_PTE_R|UPS_PTE_W|UPS_PTE_DEVICE);
 __asm__ volatile("MTSR %0,PTBR0\n\tMTSR %1,PTBR1\n\tMTSR #1,MMUCTL"::"r"((unsigned long)root0),"r"((unsigned long)root1):"memory");
}

/* Check leaf privilege as well as VA range: low identity aliases are kernel. */
int ups_pmap_user_access(unsigned long va,size_t size,int write){
 if(!size)return 1;
 if(va<4096||va>=0x0000800000000000UL||size>0x0000800000000000UL-va)return 0;
 unsigned long last=va+size-1;
 for(;;){
  unsigned long root;__asm__ volatile("MFSR PTBR0,%0":"=r"(root));
  unsigned long *table=(unsigned long *)(root&UPS_PTE_PA),pte=0;
  static const unsigned shifts[4]={39,30,21,12};
  for(unsigned level=0;level<4;level++){
   pte=table[(va>>shifts[level])&511];if(!(pte&UPS_PTE_V))return 0;
   if(level==3||(pte&UPS_PTE_LARGE))break;
   table=(unsigned long *)(pte&UPS_PTE_PA);
  }
  if((pte&0x30)!=UPS_PTE_USER||!(pte&(write?UPS_PTE_W:UPS_PTE_R)))return 0;
  if((va>>12)==(last>>12))return 1;
  va=(va&~4095UL)+4096;
 }
}

unsigned long ups_pmap_clone_bootstrap(void){
 unsigned long *root=alloc_table();memcpy(root,root0,4096);return (unsigned long)root;
}
