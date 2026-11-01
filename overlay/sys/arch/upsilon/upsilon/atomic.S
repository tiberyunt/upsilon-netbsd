/* Native NetBSD atomic primitives. SPDX-License-Identifier: BSD-2-Clause
 * Full fences are intentionally stronger than the API minimum. */
.text
.macro ALIAS name,target
 .global \name
 .set \name,\target
.endm
.global _atomic_cas_8
.type _atomic_cas_8,@function
_atomic_cas_8:
 FENCE
 CAS.b R1,R2,(R0)
 MOV.b R1,R0
 FENCE
 RET
.size _atomic_cas_8,.-_atomic_cas_8
ALIAS atomic_cas_8,_atomic_cas_8
ALIAS __sync_val_compare_and_swap_1,_atomic_cas_8
.global __sync_bool_compare_and_swap_1
__sync_bool_compare_and_swap_1:
 FENCE
 CAS.b R1,R2,(R0)
 BNE .Lcas_fail_8
 MOV.q #1,R0
 BRA .Lcas_done_8
.Lcas_fail_8:
 MOV.q #0,R0
.Lcas_done_8:
 FENCE
 RET
.global _atomic_cas_16
.type _atomic_cas_16,@function
_atomic_cas_16:
 FENCE
 CAS.w R1,R2,(R0)
 MOV.w R1,R0
 FENCE
 RET
.size _atomic_cas_16,.-_atomic_cas_16
ALIAS atomic_cas_16,_atomic_cas_16
ALIAS __sync_val_compare_and_swap_2,_atomic_cas_16
.global __sync_bool_compare_and_swap_2
__sync_bool_compare_and_swap_2:
 FENCE
 CAS.w R1,R2,(R0)
 BNE .Lcas_fail_16
 MOV.q #1,R0
 BRA .Lcas_done_16
.Lcas_fail_16:
 MOV.q #0,R0
.Lcas_done_16:
 FENCE
 RET
.global _atomic_cas_32
.type _atomic_cas_32,@function
_atomic_cas_32:
 FENCE
 CAS.l R1,R2,(R0)
 MOV.l R1,R0
 FENCE
 RET
.size _atomic_cas_32,.-_atomic_cas_32
ALIAS atomic_cas_32,_atomic_cas_32
ALIAS __sync_val_compare_and_swap_4,_atomic_cas_32
ALIAS atomic_cas_32,_atomic_cas_32
ALIAS atomic_cas_32_ni,_atomic_cas_32
ALIAS _atomic_cas_32_ni,_atomic_cas_32
ALIAS atomic_cas_uint,_atomic_cas_32
ALIAS atomic_cas_uint_ni,_atomic_cas_32
ALIAS _atomic_cas_uint,_atomic_cas_32
ALIAS _atomic_cas_uint_ni,_atomic_cas_32
.global __sync_bool_compare_and_swap_4
__sync_bool_compare_and_swap_4:
 FENCE
 CAS.l R1,R2,(R0)
 BNE .Lcas_fail_32
 MOV.q #1,R0
 BRA .Lcas_done_32
.Lcas_fail_32:
 MOV.q #0,R0
.Lcas_done_32:
 FENCE
 RET
.global _atomic_cas_64
.type _atomic_cas_64,@function
_atomic_cas_64:
 FENCE
 CAS.q R1,R2,(R0)
 MOV.q R1,R0
 FENCE
 RET
.size _atomic_cas_64,.-_atomic_cas_64
ALIAS atomic_cas_64,_atomic_cas_64
ALIAS __sync_val_compare_and_swap_8,_atomic_cas_64
ALIAS atomic_cas_64,_atomic_cas_64
ALIAS atomic_cas_64_ni,_atomic_cas_64
ALIAS _atomic_cas_64_ni,_atomic_cas_64
ALIAS atomic_cas_ulong,_atomic_cas_64
ALIAS atomic_cas_ulong_ni,_atomic_cas_64
ALIAS _atomic_cas_ulong,_atomic_cas_64
ALIAS _atomic_cas_ulong_ni,_atomic_cas_64
ALIAS atomic_cas_ptr,_atomic_cas_64
ALIAS atomic_cas_ptr_ni,_atomic_cas_64
ALIAS _atomic_cas_ptr,_atomic_cas_64
ALIAS _atomic_cas_ptr_ni,_atomic_cas_64
.global __sync_bool_compare_and_swap_8
__sync_bool_compare_and_swap_8:
 FENCE
 CAS.q R1,R2,(R0)
 BNE .Lcas_fail_64
 MOV.q #1,R0
 BRA .Lcas_done_64
.Lcas_fail_64:
 MOV.q #0,R0
.Lcas_done_64:
 FENCE
 RET
.global _atomic_add_32
.type _atomic_add_32,@function
_atomic_add_32:
 FENCE
 XADD.l R1,(R0)
 MOV.l R1,R0
 FENCE
 RET
.size _atomic_add_32,.-_atomic_add_32
ALIAS atomic_add_32,_atomic_add_32
ALIAS atomic_add_int,_atomic_add_32
ALIAS _atomic_add_int,_atomic_add_32
ALIAS __sync_fetch_and_add_4,_atomic_add_32
ALIAS __atomic_fetch_add_4,_atomic_add_32
.global _atomic_and_32
.type _atomic_and_32,@function
_atomic_and_32:
 FENCE
 ATAND.l R1,(R0)
 MOV.l R1,R0
 FENCE
 RET
.size _atomic_and_32,.-_atomic_and_32
ALIAS atomic_and_32,_atomic_and_32
ALIAS atomic_and_uint,_atomic_and_32
ALIAS _atomic_and_uint,_atomic_and_32
ALIAS __sync_fetch_and_and_4,_atomic_and_32
ALIAS __atomic_fetch_and_4,_atomic_and_32
.global _atomic_or_32
.type _atomic_or_32,@function
_atomic_or_32:
 FENCE
 ATOR.l R1,(R0)
 MOV.l R1,R0
 FENCE
 RET
.size _atomic_or_32,.-_atomic_or_32
ALIAS atomic_or_32,_atomic_or_32
ALIAS atomic_or_uint,_atomic_or_32
ALIAS _atomic_or_uint,_atomic_or_32
ALIAS __sync_fetch_and_or_4,_atomic_or_32
ALIAS __atomic_fetch_or_4,_atomic_or_32
.global _atomic_swap_32
.type _atomic_swap_32,@function
_atomic_swap_32:
 FENCE
 SWAP.l R1,(R0)
 MOV.l R1,R0
 FENCE
 RET
.size _atomic_swap_32,.-_atomic_swap_32
ALIAS atomic_swap_32,_atomic_swap_32
ALIAS atomic_swap_uint,_atomic_swap_32
ALIAS _atomic_swap_uint,_atomic_swap_32
ALIAS __sync_lock_test_and_set_4,_atomic_swap_32
ALIAS __atomic_exchange_4,_atomic_swap_32
.global __sync_lock_release_4
__sync_lock_release_4:
 FENCE
 MOV.l #0,(R0)
 FENCE
 RET
.global _atomic_add_64
.type _atomic_add_64,@function
_atomic_add_64:
 FENCE
 XADD.q R1,(R0)
 MOV.q R1,R0
 FENCE
 RET
.size _atomic_add_64,.-_atomic_add_64
ALIAS atomic_add_64,_atomic_add_64
ALIAS atomic_add_long,_atomic_add_64
ALIAS _atomic_add_long,_atomic_add_64
ALIAS atomic_add_ptr,_atomic_add_64
ALIAS _atomic_add_ptr,_atomic_add_64
ALIAS __sync_fetch_and_add_8,_atomic_add_64
ALIAS __atomic_fetch_add_8,_atomic_add_64
.global _atomic_and_64
.type _atomic_and_64,@function
_atomic_and_64:
 FENCE
 ATAND.q R1,(R0)
 MOV.q R1,R0
 FENCE
 RET
.size _atomic_and_64,.-_atomic_and_64
ALIAS atomic_and_64,_atomic_and_64
ALIAS atomic_and_ulong,_atomic_and_64
ALIAS _atomic_and_ulong,_atomic_and_64
ALIAS __sync_fetch_and_and_8,_atomic_and_64
ALIAS __atomic_fetch_and_8,_atomic_and_64
.global _atomic_or_64
.type _atomic_or_64,@function
_atomic_or_64:
 FENCE
 ATOR.q R1,(R0)
 MOV.q R1,R0
 FENCE
 RET
.size _atomic_or_64,.-_atomic_or_64
ALIAS atomic_or_64,_atomic_or_64
ALIAS atomic_or_ulong,_atomic_or_64
ALIAS _atomic_or_ulong,_atomic_or_64
ALIAS __sync_fetch_and_or_8,_atomic_or_64
ALIAS __atomic_fetch_or_8,_atomic_or_64
.global _atomic_swap_64
.type _atomic_swap_64,@function
_atomic_swap_64:
 FENCE
 SWAP.q R1,(R0)
 MOV.q R1,R0
 FENCE
 RET
.size _atomic_swap_64,.-_atomic_swap_64
ALIAS atomic_swap_64,_atomic_swap_64
ALIAS atomic_swap_ulong,_atomic_swap_64
ALIAS _atomic_swap_ulong,_atomic_swap_64
ALIAS atomic_swap_ptr,_atomic_swap_64
ALIAS _atomic_swap_ptr,_atomic_swap_64
ALIAS __sync_lock_test_and_set_8,_atomic_swap_64
ALIAS __atomic_exchange_8,_atomic_swap_64
.global __sync_lock_release_8
__sync_lock_release_8:
 FENCE
 MOV.q #0,(R0)
 FENCE
 RET
.global membar_sync
membar_sync:
 FENCE
 RET
ALIAS membar_acquire,membar_sync
ALIAS _membar_acquire,membar_sync
ALIAS membar_release,membar_sync
ALIAS _membar_release,membar_sync
ALIAS membar_producer,membar_sync
ALIAS _membar_producer,membar_sync
ALIAS membar_consumer,membar_sync
ALIAS _membar_consumer,membar_sync
ALIAS membar_enter,membar_sync
ALIAS _membar_enter,membar_sync
ALIAS membar_exit,membar_sync
ALIAS _membar_exit,membar_sync
ALIAS _membar_sync,membar_sync
ALIAS __sync_synchronize,membar_sync
