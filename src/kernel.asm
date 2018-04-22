[BITS 16]
; [ORG 0xAE00]


;===============================================
; Entry point of the kernel module.
;===============================================
kernel:
    mov     ax,cs
    mov     ds,ax
    mov     es,ax
    add     ax,100h     ; 4096 - Means the kernel may be up to 4096-bytes in size
    mov     ss,ax
    mov     sp,8000h    ; 32kb - This is the available stack space

    mov     si,msg_success
    call    print

    call    test_sant
    call    print_newline

    call    test_search
    call    print_newline

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
str_sant    db "sdfj0ßäö-_0ß34", 0Dh, 0Ah, 0
.length     equ ($-str_sant)


test_sant:
    push    bp
    mov     bp,sp
    sub     sp,str_sant.length

    lea     ax,[bp-str_sant.length]
    mov     di,ax
    mov     si,str_sant
    call    fat_sanitize

    mov     si,di
    call    print

    mov     sp,bp
    pop     bp
    ret


file_cmd    db 'CMD     BIN' 
cmd_found   db 'CMD found', 0Dh, 0Ah, 0
cmd_nfound  db 'CMD not found', 0Dh, 0Ah, 0


test_search:
    push    bp
    mov     bp,sp
    sub     sp,32       ; Will hold our FAT entry

    lea     di,[bp-32]
    mov     si,file_cmd
    call    fat_searchEntry
    test    ax,ax
    jne     .not_found

    mov     si,cmd_found
    call    print
    call    print_newline

    mov     ax,word [bp-6]
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



%include "src/kernel/string.inc"
%include "src/kernel/fat12.inc"
