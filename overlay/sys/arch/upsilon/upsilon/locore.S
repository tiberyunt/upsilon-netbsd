/* Upsilon early entry and trap glue. SPDX-License-Identifier: BSD-2-Clause */
.section .text.entry,"ax",@progbits
.global _start
_start:
 LEA @ups_boot_stack_end,SP
 MOV.q #0,FP
 JSR @ups_main
 HALT

.text
.global ups_vector_entry
ups_vector_entry:
 SUB.q #256,SP
 MOV.q R0,0(SP)
 MOV.q R1,8(SP)
 MOV.q R2,16(SP)
 MOV.q R3,24(SP)
 MOV.q R4,32(SP)
 MOV.q R5,40(SP)
 MOV.q R6,48(SP)
 MOV.q R7,56(SP)
 MOV.q R8,64(SP)
 MOV.q R9,72(SP)
 MOV.q R10,80(SP)
 MOV.q R11,88(SP)
 MOV.q R12,96(SP)
 MOV.q R13,104(SP)
 MOV.q R14,112(SP)
 MOV.q R15,120(SP)
 MOV.q R16,128(SP)
 MOV.q R17,136(SP)
 MOV.q R18,144(SP)
 MOV.q R19,152(SP)
 MOV.q R20,160(SP)
 MOV.q R21,168(SP)
 MOV.q R22,176(SP)
 MOV.q R23,184(SP)
 MOV.q R24,192(SP)
 MOV.q R25,200(SP)
 MOV.q R26,208(SP)
 MOV.q R27,216(SP)
 MOV.q R28,224(SP)
 MOV.q R29,232(SP)
 MOV.q R30,240(SP)
 LEA 272(SP),R0
 MOV.q R0,248(SP)
 MOV.q 256(SP),R0
 AND.q #0x3000,R0
 CMP.q #0x2000,R0
 BNE .Lkernelsp
 MFSR SPU,R0
 MOV.q R0,248(SP)
.Lkernelsp:
 MOV.q SP,R16
 MOV.q SP,R0
 MFSR CAUSE,R1
 MFSR FAULTADDR,R2
 MFSR FAULTINFO,R3
 AND.q #-16,SP
 JSR @ups_trap
 MOV.q R16,SP
 MFSR PSR,R0
 OR.q #0x700,R0
 MTSR R0,PSR
 MOV.q 256(SP),R0
 AND.q #0x3000,R0
 CMP.q #0x2000,R0
 BNE .Lrestore_kernel
.Lrestore_user:
 MTSR 248(SP),SPU
 MOV.q 0(SP),R0
 MOV.q 8(SP),R1
 MOV.q 16(SP),R2
 MOV.q 24(SP),R3
 MOV.q 32(SP),R4
 MOV.q 40(SP),R5
 MOV.q 48(SP),R6
 MOV.q 56(SP),R7
 MOV.q 64(SP),R8
 MOV.q 72(SP),R9
 MOV.q 80(SP),R10
 MOV.q 88(SP),R11
 MOV.q 96(SP),R12
 MOV.q 104(SP),R13
 MOV.q 112(SP),R14
 MOV.q 120(SP),R15
 MOV.q 128(SP),R16
 MOV.q 136(SP),R17
 MOV.q 144(SP),R18
 MOV.q 152(SP),R19
 MOV.q 160(SP),R20
 MOV.q 168(SP),R21
 MOV.q 176(SP),R22
 MOV.q 184(SP),R23
 MOV.q 192(SP),R24
 MOV.q 200(SP),R25
 MOV.q 208(SP),R26
 MOV.q 216(SP),R27
 MOV.q 224(SP),R28
 MOV.q 232(SP),R29
 MOV.q 240(SP),R30
 ADD.q #256,SP
 RTE
.Lrestore_kernel:
 /* A recovered kernel fault can unwind to a different stack position. */
 MOV.q 248(SP),R0
 MOV.q 256(SP),-16(R0)
 MOV.q 264(SP),-8(R0)
 MOV.q 0(SP),R0
 MOV.q 8(SP),R1
 MOV.q 16(SP),R2
 MOV.q 24(SP),R3
 MOV.q 32(SP),R4
 MOV.q 40(SP),R5
 MOV.q 48(SP),R6
 MOV.q 56(SP),R7
 MOV.q 64(SP),R8
 MOV.q 72(SP),R9
 MOV.q 80(SP),R10
 MOV.q 88(SP),R11
 MOV.q 96(SP),R12
 MOV.q 104(SP),R13
 MOV.q 112(SP),R14
 MOV.q 120(SP),R15
 MOV.q 128(SP),R16
 MOV.q 136(SP),R17
 MOV.q 144(SP),R18
 MOV.q 152(SP),R19
 MOV.q 160(SP),R20
 MOV.q 168(SP),R21
 MOV.q 176(SP),R22
 MOV.q 184(SP),R23
 MOV.q 192(SP),R24
 MOV.q 200(SP),R25
 MOV.q 208(SP),R26
 MOV.q 216(SP),R27
 MOV.q 224(SP),R28
 MOV.q 232(SP),R29
 MOV.q 240(SP),R30
 MOV.q 248(SP),SP
 SUB.q #16,SP
 RTE
.global ups_fault_probe,ups_fault_resume
ups_fault_probe:
 MOV.q @0x60000000,R0
ups_fault_resume:
 RET
.global ups_enter_user,ups_user_return
ups_enter_user:
 PUSHM #0x7fff
 MTSR #0x40002000,SPU
 PUSH.q #0x40000000
 PUSH.q #0x12000
 RTE
ups_user_return:
 POPM #0x7fff
 RET

.section .rodata,"a",@progbits
.p2align 11
.global ups_vectors
ups_vectors:
.rept 256
 .quad ups_vector_entry
.endr

.section .usertext,"ax",@progbits
.p2align 12
.global ups_user_start,ups_user_end
ups_user_start:
 MFTP R4
 CMP.q #0x40001000,R4
 BNE .Luserfail
 MOV.q #99,0(TP)
 MOV.q 0(TP),R4
 CMP.q #99,R4
 BNE .Luserfail
 MOV.q #20,R0
 SYSCALL #0
 BLO .Luserfail
 CMP.q #1,R0
 BNE .Luserfail
 MOV.q #1023,R0
 SYSCALL #0
 BHS .Luserfail
 CMP.q #78,R0
 BNE .Luserfail
 MOV.q #4,R0
 MOV.q #1,R1
 LEA .Lusermessage(PC),R2
 MOV.q #(.Luserend-.Lusermessage),R3
 SYSCALL #0
 BLO .Luserfail
 CMP.q #(.Luserend-.Lusermessage),R0
 BNE .Luserfail
 /* Kernel identity aliases must not pass copyin's user-page check. */
 MOV.q #4,R0
 MOV.q #1,R1
 MOV.q #0x200000,R2
 MOV.q #4,R3
 SYSCALL #0
 BHS .Luserfail
 CMP.q #14,R0
 BNE .Luserfail
 /* A zero-length write does not dereference its user pointer. */
 MOV.q #4,R0
 MOV.q #0,R2
 MOV.q #0,R3
 SYSCALL #0
 BLO .Luserfail
 TST.q R0
 BNE .Luserfail
 MOV.q #0,R1
 BRA .Luserexit
.Luserfail:
 MOV.q #1,R1
.Luserexit:
 MOV.q #1,R0
 SYSCALL #0
 BREAK
.Lusermessage:
 .ascii "user mode: write via copyin\n"
.Luserend:
ups_user_end:

.bss
.p2align 12
.global ups_user_stack
ups_user_stack:
 .zero 4096
.p2align 4
ups_boot_stack:
 .zero 65536
ups_boot_stack_end:

.text
.global _rtt
_rtt:
 HALT
 BRA _rtt

/* Enter a validated user trapframe on the current LWP's own kernel stack. */
.text
.global ups_user_resume
ups_user_resume:
 MOV.q R0,SP
 MFSR PSR,R0
 OR.q #0x700,R0
 MTSR R0,PSR
 BRA .Lrestore_user
