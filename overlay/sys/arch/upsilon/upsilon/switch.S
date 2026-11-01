/* Native NetBSD switch and nonlocal-return primitives. BSD-2-Clause */
#include "assym.h"
.text
.global cpu_switchto
cpu_switchto:
 MFSR PSR,R3
 MOV.q L_ADDR(R0),R5
 MOV.q L_ADDR(R1),R4
 MOV.q R3,PCB_PSR(R5)
 OR.q #0x700,R3
 MTSR R3,PSR
 MOV.q R16,PCB_SAVED+0(R5)
 MOV.q R17,PCB_SAVED+8(R5)
 MOV.q R18,PCB_SAVED+16(R5)
 MOV.q R19,PCB_SAVED+24(R5)
 MOV.q R20,PCB_SAVED+32(R5)
 MOV.q R21,PCB_SAVED+40(R5)
 MOV.q R22,PCB_SAVED+48(R5)
 MOV.q R23,PCB_SAVED+56(R5)
 MOV.q R24,PCB_SAVED+64(R5)
 MOV.q R25,PCB_SAVED+72(R5)
 MOV.q R26,PCB_SAVED+80(R5)
 MOV.q R27,PCB_SAVED+88(R5)
 MOV.q R28,PCB_SAVED+96(R5)
 MOV.q R29,PCB_SAVED+104(R5)
 MOV.q R30,PCB_SAVED+112(R5)
 MOV.q SP,PCB_SP(R5)
 MFSR TP,R3
 MOV.q R3,PCB_TP(R5)
 MFSR PTBR0,R3
 MOV.q R3,PCB_PTBR0(R5)
 MFSR ASID,R3
 MOV.q R3,PCB_ASID(R5)
 MFSR SCRATCH0,R5
 FENCE
 MOV.q R1,CI_CURLWP(R5)
 MOV.q R1,CI_ONPROC(R5)
 FENCE
 MOV.q PCB_PSR(R4),R6
 SHR3.q #8,R6,R3
 AND.q #7,R3
 MOV.l R3,CI_CPL(R5)
 MTSR PCB_TP(R4),TP
 MTSR PCB_PTBR0(R4),PTBR0
 MTSR PCB_ASID(R4),ASID
 MOV.q PCB_SP(R4),SP
 MOV.q PCB_SAVED+0(R4),R16
 MOV.q PCB_SAVED+8(R4),R17
 MOV.q PCB_SAVED+16(R4),R18
 MOV.q PCB_SAVED+24(R4),R19
 MOV.q PCB_SAVED+32(R4),R20
 MOV.q PCB_SAVED+40(R4),R21
 MOV.q PCB_SAVED+48(R4),R22
 MOV.q PCB_SAVED+56(R4),R23
 MOV.q PCB_SAVED+64(R4),R24
 MOV.q PCB_SAVED+72(R4),R25
 MOV.q PCB_SAVED+80(R4),R26
 MOV.q PCB_SAVED+88(R4),R27
 MOV.q PCB_SAVED+96(R4),R28
 MOV.q PCB_SAVED+104(R4),R29
 MOV.q PCB_SAVED+112(R4),R30
 MTSR R6,PSR
 RET

.global ups_lwp_trampoline
ups_lwp_trampoline:
 SUB.q #8,SP
 MOV.q R17,R0
 JSR (R16)
 LEA @.Lreturned,R0
 JSR @ups_fail
 HALT

.global setjmp,longjmp
setjmp:
 MFSR PSR,R1
 MOV.q R1,LB_PSR(R0)
 MOV.q R16,LB_SAVED+0(R0)
 MOV.q R17,LB_SAVED+8(R0)
 MOV.q R18,LB_SAVED+16(R0)
 MOV.q R19,LB_SAVED+24(R0)
 MOV.q R20,LB_SAVED+32(R0)
 MOV.q R21,LB_SAVED+40(R0)
 MOV.q R22,LB_SAVED+48(R0)
 MOV.q R23,LB_SAVED+56(R0)
 MOV.q R24,LB_SAVED+64(R0)
 MOV.q R25,LB_SAVED+72(R0)
 MOV.q R26,LB_SAVED+80(R0)
 MOV.q R27,LB_SAVED+88(R0)
 MOV.q R28,LB_SAVED+96(R0)
 MOV.q R29,LB_SAVED+104(R0)
 MOV.q R30,LB_SAVED+112(R0)
 LEA 8(SP),R1
 MOV.q R1,LB_SP(R0)
 MOV.q (SP),LB_PC(R0)
 MFSR TP,R1
 MOV.q R1,LB_TP(R0)
 MOV.q #0,R0
 RET
longjmp:
 MOV.q LB_PSR(R0),R2
 MTSR LB_TP(R0),TP
 MOV.q LB_SAVED+0(R0),R16
 MOV.q LB_SAVED+8(R0),R17
 MOV.q LB_SAVED+16(R0),R18
 MOV.q LB_SAVED+24(R0),R19
 MOV.q LB_SAVED+32(R0),R20
 MOV.q LB_SAVED+40(R0),R21
 MOV.q LB_SAVED+48(R0),R22
 MOV.q LB_SAVED+56(R0),R23
 MOV.q LB_SAVED+64(R0),R24
 MOV.q LB_SAVED+72(R0),R25
 MOV.q LB_SAVED+80(R0),R26
 MOV.q LB_SAVED+88(R0),R27
 MOV.q LB_SAVED+96(R0),R28
 MOV.q LB_SAVED+104(R0),R29
 MOV.q LB_SAVED+112(R0),R30
 MOV.q LB_SP(R0),SP
 PUSH.q LB_PC(R0)
 MOV.q #1,R0
 MTSR R2,PSR
 RET

/* Exercise every callee-saved register, including a deliberately odd FP. */
.global ups_verify_switch
ups_verify_switch:
 PUSHM #0x7fff
 SUB.q #16,SP
 MOV.q R2,(SP)
 ADD3.q #16,R2,R16
 ADD3.q #17,R2,R17
 ADD3.q #18,R2,R18
 ADD3.q #19,R2,R19
 ADD3.q #20,R2,R20
 ADD3.q #21,R2,R21
 ADD3.q #22,R2,R22
 ADD3.q #23,R2,R23
 ADD3.q #24,R2,R24
 ADD3.q #25,R2,R25
 ADD3.q #26,R2,R26
 ADD3.q #27,R2,R27
 ADD3.q #28,R2,R28
 ADD3.q #29,R2,R29
 ADD3.q #30,R2,R30
 JSR @cpu_switchto
 MOV.q (SP),R3
 ADD3.q #16,R3,R4
 CMP.q R4,R16
 BNE .Lbadregs
 ADD3.q #17,R3,R4
 CMP.q R4,R17
 BNE .Lbadregs
 ADD3.q #18,R3,R4
 CMP.q R4,R18
 BNE .Lbadregs
 ADD3.q #19,R3,R4
 CMP.q R4,R19
 BNE .Lbadregs
 ADD3.q #20,R3,R4
 CMP.q R4,R20
 BNE .Lbadregs
 ADD3.q #21,R3,R4
 CMP.q R4,R21
 BNE .Lbadregs
 ADD3.q #22,R3,R4
 CMP.q R4,R22
 BNE .Lbadregs
 ADD3.q #23,R3,R4
 CMP.q R4,R23
 BNE .Lbadregs
 ADD3.q #24,R3,R4
 CMP.q R4,R24
 BNE .Lbadregs
 ADD3.q #25,R3,R4
 CMP.q R4,R25
 BNE .Lbadregs
 ADD3.q #26,R3,R4
 CMP.q R4,R26
 BNE .Lbadregs
 ADD3.q #27,R3,R4
 CMP.q R4,R27
 BNE .Lbadregs
 ADD3.q #28,R3,R4
 CMP.q R4,R28
 BNE .Lbadregs
 ADD3.q #29,R3,R4
 CMP.q R4,R29
 BNE .Lbadregs
 ADD3.q #30,R3,R4
 CMP.q R4,R30
 BNE .Lbadregs
 BRA .Lverified
.Lbadregs:
 MOV.q #0,R0
.Lverified:
 ADD.q #16,SP
 POPM #0x7fff
 RET
.section .rodata
.Lreturned: .asciz "kernel thread entry returned"

/* First run of a forked LWP. R0 is the outgoing LWP from cpu_switchto.
 * The MI startup routine drops scheduler locks before the requested entry. */
.text
.global ups_fork_trampoline
ups_fork_trampoline:
 SUB.q #8,SP
 MOV.q R18,R1
 JSR @lwp_startup
 MOV.q R17,R0
 JSR (R16)
 MOV.q L_UTF(R18),R0
 JSR @ups_return_user
 HALT
