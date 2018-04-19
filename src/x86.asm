[BITS 16]
[ORG 0x7C00]
    jmp near bootstrap

;===========
; BOOTSTRAP (448-bytes)
;===========
bootstrap:
    mov     ax,07C0h
    mov     ds,ax
    mov     es,ax
    add     ax,20h
    mov     ss,ax
    mov     sp,200h

    cli

    lgdt    [gdt_descriptor]

    mov     eax,cr0
    or      eax,1
    mov     cr0,eax

    jmp     GDT_CODE:pmode      ; VIOLATION

 

[BITS 32]
pmode:
    mov     ax,GDT_DATA
    mov     ds,ax
    mov     ss,ax
    mov     es,ax
    mov     fs,ax
    mov     gs,ax
    mov     esp,10000h
    hlt


;===============================================
; GLOBAL DESCRIPTOR TABLE (GDT)
;===============================================
gdt_start:

    gdt_null:
        dd  0           ; null descriptor  
        dd  0

    gdt_code:
        dw  0FFFFh      ; limit_low
        dw  0           ; base_low
        db  0           ; base_middle
        db  10011010b   ; access
        db  11001111b   ; granularity
        db  0           ; base_high

    gdt_data:
        dw  0FFFFh      ; limit_low
        dw  0           ; base_low
        db  0           ; base_middle
        db  10010010b   ; access
        db  11001111b   ; granularity
        db  0           ; base_high

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

GDT_CODE    equ gdt_code - gdt_start
GDT_DATA    equ gdt_data - gdt_start


;===========
; BOOT SIG (2-bytes)
;===========
times 510-($-$$) db 0
dw 0AA55h
