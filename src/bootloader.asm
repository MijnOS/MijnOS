BITS 16                             ; The bootloader always starts in 16-bit mode
    jmp     07C0h:start             ; Always make a jump first, this will set
                                    ; the CS and IP register values and allows
                                    ; for a Microsoft compliant FAT12 table.

;===========
; CONSTANTS
;===========
%define KERNEL_DRIVE    0           ; Floppy drive 0
%define KERNEL_SEG      08E0h       ; Put it in the segment behind our
%define KERNEL_LOC      00h         ; bootloader with an offset of 0.
%define KERNEL_SECTORS  1           ; The kernel only comprises 1 sector


;===========
; BOOT CODE
;===========
start:
    mov     ax,cs                   ; Set the DS and ES segment register
    mov     ds,ax                   ; to the same position as the CS register.
    mov     es,ax
    add     ax,32                   ; Sets up a 4096-bytes stack behind our
    mov     ss,ax                   ; bootloader. (0x7C00 + 512)
    mov     sp,4096                 ; Set the pointer to the LAST byte.
                                    ; [ss:sp] = 0x7C00 + 512 + 4096
    push    es
    mov     ax,KERNEL_SEG
    mov     es,ax
    call    loadKernel
    pop     es

    jmp     KERNEL_SEG:KERNEL_LOC   ; Call the kernel
    jmp     $                       ; Fail-safe

;===========
; void loadKernel(void)
;   @param ah CONST BYTE 02h
;   @param es The segment to read the data too.
;   @param al The number of sectors to read.
;   @param bx The address to copy the read data too.
;   @param ch The cylinder to read from.
;   @param cl The sector to read. (NOTE: This begins at sector 1 instead of 0!)
;   @param dl The drive to load from.
;   @param dh The drive's head.
;   @return cf Set on error/cleared on success.
;   @return ah The return code.
;   @return al The actual number of sectors read.
;===========
loadKernel:
    pusha

    mov     ah,02h
    mov     al,KERNEL_SECTORS
    mov     ch,0
    mov     cl,2
    mov     dh,0
    mov     dl,KERNEL_DRIVE
    mov     bx,KERNEL_LOC

    int     13h
    jc      .failed

    cmp     al,KERNEL_SECTORS
    jne     .failed

    popa
    ret

.failed:
    mov     ah,0Eh
    mov     al,41h
    int     10h
    jmp     $


;===========
; BOOT SIG
;===========
times 510-($-$$) db 0
dw 0xAA55
