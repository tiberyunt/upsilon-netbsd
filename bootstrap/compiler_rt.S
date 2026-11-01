/* Native integer compiler support for the BSD bootstrap. BSD-2-Clause */
.text
.global __udivdi3,__umoddi3,__divdi3,__moddi3,__udivsi3,__umodsi3,__divsi3,__modsi3
__udivdi3:
 MOV.q R0,R2
 CLR.q R3
 UDIVW.q R1,R2
 MOV.q R2,R0
 RET
__umoddi3:
 MOV.q R0,R2
 CLR.q R3
 UDIVW.q R1,R2
 MOV.q R3,R0
 RET
__divdi3:
 DIV3.q R1,R0,R0
 RET
__moddi3:
 DIV3.q R1,R0,R2
 MUL3.q R1,R2,R2
 SUB.q R2,R0
 RET
__udivsi3:
 MOV.l R0,R0
 MOV.l R1,R1
 BRA __udivdi3
__umodsi3:
 MOV.l R0,R0
 MOV.l R1,R1
 BRA __umoddi3
__divsi3:
 MOVSX.l.q R0,R0
 MOVSX.l.q R1,R1
 BRA __divdi3
__modsi3:
 MOVSX.l.q R0,R0
 MOVSX.l.q R1,R1
 BRA __moddi3
.global __bswapdi2,__bswapsi2
__bswapdi2:
 BSWAP.q R0,R0
 RET
__bswapsi2:
 BSWAP.l R0,R0
 RET
