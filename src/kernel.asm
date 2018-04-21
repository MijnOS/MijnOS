[BITS 16]


;===============================================
; Entry point of the kernel module.
;===============================================
kernel:
    mov     ax,cs
    mov     ds,ax
    mov     es,ax
    add     ax,32
    mov     ss,ax
    mov     sp,4096

    mov     si,msg_success
    call    print

    jmp     $


;===========
; void print(const char *si)
;   @param si A pointer to the string to print.
;===========
print:
    mov     ah,0Eh

.repeat:
    lodsb
    cmp     al,0
    je      .return
    int     10h
    jmp     .repeat

.return:
    ret

;===========
; STRINGS
;===========
msg_success db "Kernel reports 0 errors." ,0Dh,0Ah,0


%include "src/kernel/fat12.inc"
