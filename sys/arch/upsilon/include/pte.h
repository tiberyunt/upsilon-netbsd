/* Upsilon v0.4 4-level radix PTEs. SPDX-License-Identifier: BSD-2-Clause */
#ifndef _UPSILON_PTE_H_
#define _UPSILON_PTE_H_
#define UPS_PTE_V 0x001UL
#define UPS_PTE_R 0x002UL
#define UPS_PTE_W 0x004UL
#define UPS_PTE_X 0x008UL
#define UPS_PTE_USER 0x020UL
#define UPS_PTE_A 0x040UL
#define UPS_PTE_D 0x080UL
#define UPS_PTE_G 0x100UL
#define UPS_PTE_LARGE 0x200UL
#define UPS_PTE_DEVICE (2UL << 52)
#define UPS_PTE_PA 0x000ffffffffff000UL
#endif
