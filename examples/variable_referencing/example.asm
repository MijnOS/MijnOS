[BITS 32]

SECTION .text align=16
global _varUnnamed
global _varNamed

g_variable  dd 011223344h, 055667788h

_varUnnamed:
    mov     eax,dword [$-4]         ; Unnamed but using memory address instead
    ret

_varNamed:
    mov     eax,dword [g_variable]  ; Named variable approach
    ret
