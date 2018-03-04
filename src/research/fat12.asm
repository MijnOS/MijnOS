; BPB
%define OEM_ID                      'MijnOS_0'      ; OEM identifier
%define BYTES_PER_SECTOR            512             ; Number of bytes per sector
%define SECTORS_PER_CLUSTER         1               ; Number of sectors per cluster
%define RESERVED_SECTORS            1               ; Number of reserved sectors
%define NUMBER_OF_FATS              2               ; Number of FAT tables
%define MAX_ROOT_ENTRIES            224             ; Maximum number of root directories
%define NUM_SMALL_SECTORS           2880            ; Total sector count (For FAT16 and older)
%define MEDIA_DESCRIPTOR            0F0h            ; 3.5" 1.44MB Floppy
%define SECTORS_PERS_FAT            9               ; Sectors per FAT
%define SECTORS_PER_TRACK           18              ; Sectors per track
%define NUMBER_OF_HEADS             2               ; Number of heads
%define HIDDEN_SECTORS              0               ; Number of hidden sectors
%define NUM_LARGE_SECTORS           0               ; Total sector count (For FAT32 and newer)

; Extended BPB
%define PHYSICAL_DRIVE_NUMBER       0               ; Physical drive number
%define EBPB_RESERVED               0               ; Reserved
%define BOOT_SIGNATURE              029h            ; Boot signature, indicates the presence of the following three fields
%define VOLUME_SERIAL_NUMBER        22352E33h       ; Volume id
%define VOLUME_LABEL                'NO NAME    '   ; Volume label
%define FILE_SYSTEM_TYPE            'FAT12   '      ; File system type


[BITS 16]
    jmp near bootstrap

;===========
; FAT12
;===========
db OEM_ID
dw BYTES_PER_SECTOR
db SECTORS_PER_CLUSTER
dw RESERVED_SECTORS
db NUMBER_OF_FATS
dw MAX_ROOT_ENTRIES
dw NUM_SMALL_SECTORS
db MEDIA_DESCRIPTOR
dw SECTORS_PERS_FAT
dw SECTORS_PER_TRACK
dw NUMBER_OF_HEADS
dd HIDDEN_SECTORS
dd NUM_LARGE_SECTORS
db PHYSICAL_DRIVE_NUMBER
db EBPB_RESERVED
db BOOT_SIGNATURE
dd VOLUME_SERIAL_NUMBER
db VOLUME_LABEL
db FILE_SYSTEM_TYPE

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
; FAT12
;===========
%define DEF_FAT_ROOT    512+(NUMBER_OF_FATS * SECTORS_PERS_FAT * BYTES_PER_SECTOR)
off_fat_root dw DEF_FAT_ROOT


fat:
    ret

;===========
; BOOT SIG (2-bytes)
;===========
times 510-($-$$) db 0
sign dw 0AA55h


;===============================================================================
; FILE ALLOCATION TABLE (FAT TABLE)
;===============================================================================
; NOTE: This should be done by the C++ program and not manually programmed here.
