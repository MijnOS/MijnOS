[BITS 16]
[ORG 07C0h]
    jmp near start

;===========
; OEM ID (8-bytes)
;===========
oem_id db 'MijnOS_0'

;===========
; BPB (25-bytes)
;===========
bytes_per_sector        dw 0002h
sectors_per_cluster     db 040h
reserved_sectors        dw 0100h
number_of_fats          db 02h
root_entries            dw 0002h
small_sectors           dw 0000h
media_descriptor        db 0F8h      ; High density floppy...
sectors_per_fat         dw 0FC00h
sectors_per_track       dw 03F00h
number_of_heads         dw 04000h
hidden_sectors          dd 03F000000h
large_sectors           dd 01F03E00h

;===========
; Extended BPB (26-bytes)
;===========
physical_drive_number   db 080h
reserved                db 00h
extended_boot_signature db 029h
volume_serial_number    dd 0A88B3652h
volume_label            db 'NO NAME', 0,0,0,0
file_system_type        db 'FAT12', 0,0,0

;===========
; BOOTSTRAP (448-bytes)
;===========
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
