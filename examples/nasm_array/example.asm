[BITS 32]

SECTION .data align=16
g_array dw 01234h, 05678h


SECTION .text align=16
global _narray

_narray:
    mov     eax,dword [esp+4]       ; index
    shl     eax,1                   ; index * sizeof(WORD)
    add     eax,g_array             ; g_array + index
    movzx   eax,word [eax]
    ret
