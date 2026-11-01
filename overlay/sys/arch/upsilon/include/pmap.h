/* Early UP pmap implementation. SPDX-License-Identifier: BSD-2-Clause */
#ifndef _UPSILON_PMAP_H_
#define _UPSILON_PMAP_H_
#include <sys/types.h>
/* Including this header directly must also supply the MI pmap types. */
#ifndef _PMAP_VM_
#include <uvm/uvm_pmap.h>
#endif
struct ups_pv_entry;
#define __HAVE_VM_PAGE_MD 1
struct vm_page_md { struct ups_pv_entry *md_pv; unsigned long md_attrs; };
struct pmap {
 paddr_t pm_root;
 struct pmap_statistics pm_stats;
 struct ups_pv_entry *pm_mappings;
 unsigned int pm_refs,pm_asid;
 bool pm_kernel;
};
#define pmap_resident_count(pm) ((pm)->pm_stats.resident_count)
#define pmap_wired_count(pm) ((pm)->pm_stats.wired_count)
/* Bootstrap providers; full UVM allocator integration is not installed yet. */
void ups_pmap_init(paddr_t,paddr_t,psize_t,struct vm_page *(*)(paddr_t));
void ups_pmap_page_init(struct vm_page *);
unsigned int ups_pmap_table_usage(void);
unsigned int ups_pmap_mapping_usage(void);
unsigned long ups_pmap_query(pmap_t,vaddr_t);
#endif
