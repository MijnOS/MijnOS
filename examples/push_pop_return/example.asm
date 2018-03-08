[BITS 32]

SECTION .text align=16
global _pushpop

_pushpop:
    push    dword 011223344h
    push    dword 055667788h
    pop     eax
    pop     edx
    ret
