[BITS 32]

SECTION .text align=16
global _paramEBP
global _paramESP


_paramEBP:
    push    ebp
    mov     ebp,esp

    ; 1) Get the value of arg1 and push it onto the stack
    mov     eax,dword [ebp+8]       ; arg1
    mov     edx,dword [eax]         ; value1
    push    edx

    ; 2) Set the value of arg1 to the value of arg2
    mov     eax,dword [ebp+8]       ; arg1
    mov     ecx,dword [ebp+0Ch]     ; arg2
    mov     edx,dword [ecx]         ; value2
    mov     dword [eax],edx

    ; 3) Pop the original value of arg1 the stack and put it into arg2
    pop     edx                     ; value1
    mov     ecx,dword [ebp+0Ch]     ; arg2
    mov     dword [ecx],edx

    mov     esp,ebp
    pop     ebp
    ret


_paramESP:

    ; 1) Get the value of arg1 and push it onto the stack
    mov     eax,dword [esp+4]       ; arg1
    mov     edx,dword [eax]         ; value1
    push    edx

    ; 2) Set the value of arg1 to the value of arg2
    mov     eax,dword [esp+8]       ; arg1
    mov     ecx,dword [esp+0Ch]     ; arg2
    mov     edx,dword [ecx]         ; value2
    mov     dword [eax],edx

    ; 3) Pop the original value of arg1 the stack and put it into arg2
    pop     edx
    mov     ecx,dword [esp+8]       ; arg2
    mov     dword [ecx],edx

    ret
