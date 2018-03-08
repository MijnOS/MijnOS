[BITS 32]

;===========
; Required by Microsoft's linker.
;===========
SECTION .text align=16
global _functieASM

;===========
; int functieASM(int x, int y)
;===========
_functieASM:
    push    ebp                     ; Preserve the stack pointer of the caller
    mov     ebp,esp                 ; This will be our stack pointer
    sub     esp,4                   ; Reserve space for a DWORD
    push    ecx                     ; Preserve the value of ECX
    mov     eax,dword [ebp+8]       ; Argument #1 - x
    mov     ecx,dword [ebp+0Ch]     ; Argument #2 - y
    imul    ecx                     ; Multiply x*y 
    mov     dword [ebp-4],eax       ; Store the result in local variable c
    mov     eax,dword [ebp-4]       ; Set the return value to be the result
    pop     ecx                     ; Restore the value of ECX
    add     esp,4                   ; This will give back the space used by our local variable
    mov     esp,ebp                 ; Restore the stack to its initial state
    pop     ebp                     ; Restore the stack frame pointer of the caller
    ret                             ; Return to the caller
