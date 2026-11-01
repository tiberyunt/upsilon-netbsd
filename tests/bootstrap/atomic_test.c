/* Native and upstream NetBSD atomic operations, executed on Upsilon.
 * SPDX-License-Identifier: BSD-2-Clause */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <machine/intr.h>
#include <sys/atomic.h>
#include <machine/lock.h>
#include "boot.h"
static void check(bool ok,const char *why){if(!ok)ups_fail(why);}
static volatile uint32_t count32;
static volatile uint64_t count64;
static volatile unsigned interrupts;
static void clock_hook(void){
 atomic_inc_32(&count32);atomic_inc_64(&count64);interrupts++;
 ups_compare(ups_cycles()+5000);
}
#define TEST_WIDTH(bits,type) do { \
 struct {type before;volatile type value;type after;} a={17,(type)~(type)0,23}; \
 check(atomic_cas_##bits(&a.value,(type)~(type)0,11)==(type)~(type)0&&a.value==11,"CAS success " #bits); \
 check(atomic_cas_##bits(&a.value,12,19)==11&&a.value==11,"CAS miss " #bits); \
 check(atomic_cas_##bits##_ni(&a.value,11,20)==11&&a.value==20,"CAS ni " #bits); \
 check(atomic_swap_##bits(&a.value,7)==20&&a.value==7,"swap old value " #bits); \
 atomic_add_##bits(&a.value,-9);check(a.value==(type)-2,"negative add/wrap " #bits); \
 check(atomic_add_##bits##_nv(&a.value,4)==2&&a.value==2,"add new value " #bits); \
 atomic_or_##bits(&a.value,0x90);check(a.value==0x92,"OR " #bits); \
 check(atomic_or_##bits##_nv(&a.value,0x04)==0x96,"OR new value " #bits); \
 atomic_and_##bits(&a.value,0x8f);check(a.value==0x86,"AND " #bits); \
 check(atomic_and_##bits##_nv(&a.value,0x07)==6,"AND new value " #bits); \
 atomic_inc_##bits(&a.value);check(atomic_inc_##bits##_nv(&a.value)==8,"increment " #bits); \
 atomic_dec_##bits(&a.value);check(atomic_dec_##bits##_nv(&a.value)==6,"decrement " #bits); \
 check(a.before==17&&a.after==23,"atomic neighbor preservation " #bits); \
 check(__sync_val_compare_and_swap(&a.value,6,10)==6&&a.value==10,"builtin CAS " #bits); \
 check(!__sync_bool_compare_and_swap(&a.value,6,20)&&__sync_bool_compare_and_swap(&a.value,10,30),"builtin bool CAS " #bits); \
 check(__sync_fetch_and_add(&a.value,3)==30&&a.value==33,"builtin fetch/add " #bits); \
 check(__sync_add_and_fetch(&a.value,2)==35,"builtin add/fetch " #bits); \
 check(__sync_fetch_and_or(&a.value,0x80)==35&&a.value==0xa3,"builtin fetch/or " #bits); \
 check(__sync_or_and_fetch(&a.value,4)==0xa7,"builtin or/fetch " #bits); \
 check(__sync_fetch_and_and(&a.value,0x0f)==0xa7&&a.value==7,"builtin fetch/and " #bits); \
 check(__sync_and_and_fetch(&a.value,3)==3,"builtin and/fetch " #bits); \
 check(__sync_lock_test_and_set(&a.value,9)==3&&a.value==9,"builtin exchange " #bits); \
 __sync_lock_release(&a.value);check(a.value==0,"builtin release " #bits); \
} while(0)
void ups_atomic_tests(void){
 TEST_WIDTH(32,uint32_t);TEST_WIDTH(64,uint64_t);
 struct {uint8_t lo;volatile uint8_t v;uint8_t hi;} small={19,0x80,23};
 check(atomic_cas_8(&small.v,0x80,7)==0x80&&small.v==7&&atomic_cas_8(&small.v,9,4)==7,"byte CAS");
 struct {uint16_t lo;volatile uint16_t v;uint16_t hi;} half={31,0x8000,47};
 check(atomic_cas_16(&half.v,0x8000,7)==0x8000&&half.v==7&&atomic_cas_16(&half.v,9,4)==7,"halfword CAS");
 check(small.lo==19&&small.hi==23&&half.lo==31&&half.hi==47,"subword CAS neighbor preservation");
 volatile unsigned int u=10;volatile unsigned long l=10;
 atomic_add_int(&u,-2);check(atomic_add_int_nv(&u,3)==11,"int add alias");
 atomic_and_uint(&u,7);check(atomic_or_uint_nv(&u,8)==11&&atomic_and_uint_nv(&u,15)==11,"uint logical aliases");
 atomic_or_uint(&u,16);check(atomic_cas_uint(&u,27,1)==27&&atomic_cas_uint_ni(&u,1,2)==1&&atomic_swap_uint(&u,5)==2,"uint CAS/swap aliases");
 atomic_inc_uint(&u);atomic_dec_uint(&u);check(atomic_inc_uint_nv(&u)==6&&atomic_dec_uint_nv(&u)==5,"uint inc/dec aliases");
 atomic_add_long(&l,-2);check(atomic_add_long_nv(&l,3)==11,"long add alias");
 atomic_and_ulong(&l,7);check(atomic_or_ulong_nv(&l,8)==11&&atomic_and_ulong_nv(&l,15)==11,"ulong logical aliases");
 atomic_or_ulong(&l,16);check(atomic_cas_ulong(&l,27,1)==27&&atomic_cas_ulong_ni(&l,1,2)==1&&atomic_swap_ulong(&l,5)==2,"ulong CAS/swap aliases");
 atomic_inc_ulong(&l);atomic_dec_ulong(&l);check(atomic_inc_ulong_nv(&l)==6&&atomic_dec_ulong_nv(&l)==5,"ulong inc/dec aliases");
 char bytes[64];void *volatile p=bytes+10;
 atomic_add_ptr(&p,-2);check(atomic_add_ptr_nv(&p,3)==bytes+11,"pointer byte addition");
 check(atomic_cas_ptr(&p,bytes+11,bytes+20)==bytes+11&&atomic_cas_ptr_ni(&p,bytes+19,bytes+30)==bytes+20,"pointer CAS aliases");
 check(atomic_swap_ptr(&p,bytes+40)==bytes+20,"pointer swap alias");
 atomic_inc_ptr(&p);atomic_dec_ptr(&p);check(atomic_inc_ptr_nv(&p)==bytes+41&&atomic_dec_ptr_nv(&p)==bytes+40,"pointer inc/dec aliases");
 membar_acquire();membar_release();membar_producer();membar_consumer();membar_enter();membar_exit();membar_sync();__sync_synchronize();
 volatile unsigned publication=0;atomic_store_release(&publication,37);check(atomic_load_acquire(&publication)==37,"release/acquire accessors");
 __cpu_simple_lock_t lock;__cpu_simple_lock_init(&lock);
 check(__cpu_simple_lock_try(&lock)&&!__cpu_simple_lock_try(&lock),"atomic spinlock try/busy");__cpu_simple_unlock(&lock);
 check(__cpu_simple_lock_try(&lock),"atomic spinlock reuse");__cpu_simple_unlock(&lock);
 /* Interrupt and mainline both update the same objects. No IPL masking is
  * added around the CAS-loop routines. An explicit stale-CAS case ensures
  * that interrupt updates cannot be overwritten by a pre-interrupt value. */
 int saved=splhigh();count32=0;count64=0;interrupts=0;ups_clock_test_hook=clock_hook;
 ups_compare(ups_cycles()+5000);spl0();__asm__ volatile("WAIT":::"memory");splhigh();
 check(interrupts>0&&atomic_cas_32(&count32,0,99)==count32&&count32!=99,"interrupt stale CAS protection");
 unsigned before=interrupts;
 spl0();
 for(unsigned i=0;i<200;i++){atomic_add_32_nv(&count32,1);atomic_add_64_nv(&count64,1);}
 splhigh();ups_compare(~0UL);ups_clock_test_hook=NULL;
 check(interrupts>before,"interrupts during atomic loops");
 check(count32==200+interrupts&&count64==200+interrupts,"interrupt/mainline atomic counts");
 check(curcpu()->ci_intr_depth==0,"atomic IRQ depth");splx(saved);
 printf("atomics: native CAS, upstream RMW, aliases, builtins, barriers and IRQ sharing passed\n");
}
