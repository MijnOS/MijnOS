[BITS 32]
%define COMPILE_CONSTANT        1

; Variable
var1 db COMPILE_CONSTANT
var2 dw COMPILE_CONSTANT

start:
    mov     eax,0
    mov     edx,dword [eax]
    ret

