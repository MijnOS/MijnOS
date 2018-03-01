;===========
; https://technet.microsoft.com/en-us/library/cc976796.aspx
;===========
[BITS 16]
    jmp near bootstrap

;===========
; OEM ID (8-bytes)
;===========
oem_id db 'MijnOS_0'

;===========
; BPB (25-bytes)
;===========
bytes_per_sector        dw 0002h        ; 512-bytes per sector
sectors_per_cluster     db 040h         ; 1 sector per cluster
reserved_sectors        dw 0100h        ; Number of reserved sectors
number_of_fats          db 02h          ; Number of FATs is 2
root_entries            dw 0002h        ; Maximum number of root directory entries
small_sectors           dw 0000h        ; Total sector count
media_descriptor        db 0F8h         ; Switch to Floppy...
sectors_per_fat         dw 0FC00h       ; Sectors per FAT
sectors_per_track       dw 03F00h       ; Sectors per track
number_of_heads         dw 04000h       ; Number of heads
hidden_sectors          dd 03F000000h   ; Number of hidden sectors is 0
large_sectors           dd 01F03E00h    ; Total sector count (0 for FAT12)

;===========
; Extended BPB (26-bytes)
;===========
physical_drive_number   db 080h         ; Physical drive number
reserved                db 00h          ; Reserved
extended_boot_signature db 029h         ; Boot signature, indicates the presence of the following three fields
volume_serial_number    dd 0A88B3651h   ; Volume id (for quick eject, etc.)
volume_label            db 'NO NAME', 020h, 020h, 020h, 020h    ; Volume label
file_system_type        db 'FAT12', 020h, 020h, 020h            ; File system type

;===========
; BOOTSTRAP (448-bytes)
;===========
bootstrap:
    jmp     07C0h:start

start:
    mov     ax,cs
    mov     ds,ax
    mov     es,ax

    jmp     $

;===========
; BOOT SIG (2-bytes)
;===========
times 510-($-$$) db 0
sign dw 0AA55h


;===============================================================================
; FILE ALLOCATION TABLE (FAT TABLE)
;===============================================================================
; NOTE: This should be done by the C++ program and not manually programmed here.
