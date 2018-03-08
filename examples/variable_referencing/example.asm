[BITS 32]

SECTION .text align=16
global _varUnnamed
global _varNamed

g_variable  dd 012345678h

_varUnnamed:
    mov     eax,dword [$-4]         ; Unnamed but using memory address instead
    ret

_varNamed:
    mov     eax,dword [g_variable]  ; Named variable approach
    ret
