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

    ;call    test_loadSector
    ;call    print_newline

    call    test_search
    call    print_newline

    jmp     $


;===========
; STRINGS
;===========
msg_success db "Kernel reports 0 errors." ,0Dh,0Ah,0
msg_failed  db "Kernel reports 1 error." ,0Dh,0Ah,0
.length     equ ($-msg_success)

file_cmd    db 'CMD     BIN' 
cmd_found   db 'CMD found', 0Dh, 0Ah, 0
cmd_nfound  db 'CMD not found', 0Dh, 0Ah, 0

str_called  db 'CALLED', 0Dh, 0Ah, 0


test_search:
    push    bp
    mov     bp,sp
    sub     sp,32       ; Will hold our FAT entry

    lea     di,[bp-32]
    mov     si,file_cmd

    push    es
    push    bx

    mov     bx,ss
    mov     es,bx

    push    si
    push    di
    call    fat_searchEntry
    add     sp,4

    pop     bx
    pop     es

    test    ax,ax
    jne     .not_found

    mov     si,cmd_found
    call    print
    call    print_newline

    push    ds
    push    bx
    mov     bx,ss
    mov     ds,bx
    mov     word [bp-21],0
    lea     ax,[bp-32]
    mov     si,ax
    call    print
    call    print_newline
    pop     bx
    pop     ds

    mov     ax,word [bp-6]          ; 0500h
    call    print_hex
    call    print_newline

    jmp     .return


.not_found:
    mov     si,cmd_nfound
    call    print

.return:
    mov     sp,bp
    pop     bp
    ret


test_loadSector:
    push    bp
    mov     bp,sp
    sub     sp,512
    push    di

    push    es
    push    bx

    mov     bx,ss
    mov     es,bx

    lea     di,[bp-512]
    mov     ax,0
    call    fat_loadRootSector

    pop     bx
    pop     es

    mov     ax,word [bp-512]
    call    print_hex

    mov     ax,word [bp-480]
    call    print_hex

    mov     ax,word [bp-448]
    call    print_hex

    mov     ax,word [bp-416]
    call    print_hex

    pop     di
    mov     sp,bp
    pop     bp
    ret



%include "src/kernel/std.inc"
%include "src/kernel/fat12.inc"
