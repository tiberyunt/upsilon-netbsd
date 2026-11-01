/* User programs reached through the fork trampoline. BSD-2-Clause */
.section .forkusertext,"ax",@progbits
.p2align 12
.global ups_fork_user_start,ups_fork_user_exec,ups_fork_user_end
ups_fork_user_start:
 BLO .Lfail
 TST.q R0
 BNE .Lfail
 CMP.q #1,R1
 BNE .Lfail
 CMP.q #0x1122334455667788,R20
 BNE .Lfail
 CMP.q #0x70001fe0,SP
 BNE .Lfail
 MFTP R5
 CMP.q #0x70001020,R5
 BNE .Lfail
 MOV.q #123,0(TP)
 BRA .Lcommon
ups_fork_user_exec:
 TST.q R0
 BNE .Lfail
 CMP.q #0x70001080,R1
 BNE .Lfail
 TST.q R20
 BNE .Lfail
 CMP.q #0x70001ff0,SP
 BNE .Lfail
 MFTP R5
 TST.q R5
 BNE .Lfail
.Lcommon:
 MOV.q (SP),R5
 CMP.q #42,R5
 BNE .Lfail
 MOV.q #20,R0
 SYSCALL #0
 BLO .Lfail
 CMP.q #1,R0
 BNE .Lfail
 MOV.q #0,R1
 BRA .Lexit
.Lfail:
 MOV.q #1,R1
.Lexit:
 MOV.q #1,R0
 SYSCALL #0
 BREAK
ups_fork_user_end:
