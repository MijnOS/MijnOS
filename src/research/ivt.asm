BITS 16

struc gdt_entry_struct

    limit_low:   resb 2
    base_low:    resb 2
    base_middle: resb 1
    access:      resb 1
    granularity: resb 1
    base_high:   resb 1

endstruc

ptr_gdtr    db 0,0,0, 0,0,0

flush_gdt:
    lgdt [ptr_gdtr]
    jmp 0x08:complete_flush
 
complete_flush:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret


