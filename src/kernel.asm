[BITS 16]
; [ORG 0xAE00]



;===============================================
; Entry point of the kernel module.
;===============================================
kernel:
    mov     ax,cs
    mov     ds,ax
    mov     es,ax
    add     ax,100h
    mov     ss,ax
    mov     sp,8000h
    ;mov     ss,ax
    ;mov     sp,9000h
    ;  4kb - 1000h - This is the maximum size of the kernel.
    ; 32kb - 8000h - This is the available stack space

    mov     si,msg_success
    call    print

    ; call    test_common
    ; call    print_newline

    call    register_interrupts

.debug:
    push    es
    push    bx
    mov     bx,0
    mov     es,bx
    mov     bx,70h
    shl     bx,2
    mov     ax,word [es:bx]
    pop     bx
    pop     es
    call    print_hex

    mov     ah,0Eh
    mov     al,3Ah
    int     10h

    push    es
    push    bx
    mov     bx,0
    mov     es,bx
    mov     bx,70h
    shl     bx,2
    add     bx,2
    mov     ax,word [es:bx]
    pop     bx
    pop     es
    call    print_hex


.test:
    mov     ax,test_var
    mov     bx,ax
    mov     word [ds:bx],05678h

    mov     ax,word [ds:bx]
    call    print_hex

    mov     ax,test_var
    int     70h                 ; Immediately test

    mov     ax,word [ds:bx]
    call    print_hex

    mov     si,msg_success
    call    print

.keypress:
    mov     ah,00h
    int 	16h
    movzx   ax,al
    call    print_hex
    call    print_newline
    jmp     .keypress

    jmp     $


test_var    dw 0

;===============================================
; N/A
;   In:
;     N/A
;   Out:
;     N/A
;===============================================
register_interrupts:
    pusha
    push    ds
    push    es

    ;mov     ax,test_interrupt
    ;mov     cx,ds

    mov     bx,0        ; We need to address zero
    mov     es,bx

    mov     bx,01C0h    ; int 70h - Like Linux systems

    mov     word [es:bx],test_interrupt
    mov     word [es:bx+2],0AE0h

    pop     es
    pop     ds
    popa
    ret


;===============================================
; N/A
;   In:
;     N/A
;   Out:
;     N/A
;===============================================
test_interrupt:
    push    bx
    mov     bx,ax
    mov     word [ds:bx],01234h
    pop     bx
    push    ax
    mov     ah,0Eh
    mov     al,41h
    int     10h
    mov     al,0Dh
    int     10h
    mov     al,0Ah
    int     10h
    pop     ax
    iret


;===========
; STRINGS
;===========
msg_success db "Kernel reports 0 errors.", 0Dh, 0Ah, 0



%include "src/kernel/const.inc"
%include "src/kernel/std.inc"
%include "src/kernel/fat12.inc"
%include "src/kernel/tests.inc"
