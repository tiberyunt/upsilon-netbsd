/* Fault-contained copyin/out using per-LWP onfault labels. BSD-2-Clause */
#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <machine/frame.h>
#include <machine/vmparam.h>
extern int ups_pmap_user_access(unsigned long,size_t,int);
static int user_range(const void *p,size_t n){
 uintptr_t a=(uintptr_t)p;return n==0||(a>=PAGE_SIZE&&a<VM_MAXUSER_ADDRESS&&n<=VM_MAXUSER_ADDRESS-a);
}
static void psr_restore(unsigned long n){__asm__ volatile("MTSR %0,PSR"::"r"(n):"memory");}
static __attribute__((noinline,noclone)) void copy_bytes(const void *src,void *dst,size_t n){
 const volatile unsigned char *s=src;volatile unsigned char *d=dst;
 while(n--)*d++=*s++;
}
static int do_copy(const void *src,void *dst,size_t n,bool out){
 if(!user_range(out?dst:src,n))return EFAULT;
 if(!n)return 0;
 if(!ups_pmap_user_access((uintptr_t)(out?dst:src),n,out))return EFAULT;
 label_t f;label_t *saved=curlwp->l_md.md_onfault;
 int error=setjmp(&f);
 if(error){curlwp->l_md.md_onfault=saved;return error;}
 curlwp->l_md.md_onfault=&f;
 __asm__ volatile("UAON":::"memory");
 copy_bytes(src,dst,n);
 psr_restore(f.lb_psr);curlwp->l_md.md_onfault=saved;
 return 0;
}
int copyin(const void *u,void *k,size_t n){return do_copy(u,k,n,false);}
int copyout(const void *k,void *u,size_t n){return do_copy(k,u,n,true);}
