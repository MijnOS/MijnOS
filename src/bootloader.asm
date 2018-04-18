[BITS 16]
    jmp near bootstrap



; BPB
OEMId                   db 'MijnOS_0'           ; OEM identifier
BytesPerSector          dw 512                  ; Number of bytes per sector
SectorsPerCluster       db 1                    ; Number of sectors per cluster
ReservedSectors         dw 1                    ; Number of reserved sectors
NumberOfFats            db 2                    ; Number of FAT tables
MaxRootEntries          dw 224                  ; Maximum number of root directories
SmallSectors            dw 2880                 ; Total sector count (For FAT16 and older)
MediaDescriptor         db 0F0h                 ; 3.5" 1.44MB Floppy
SectorsPerFat           dw 9                    ; Sectors per FAT
SectorsPerTrack         dw 18                   ; Sectors per track
NumberOfHeads           dw 2                    ; Number of heads
HiddenSectors           dd 0                    ; Number of hidden sectors
LargeSectors            dd 0                    ; Total sector count (For FAT32 and newer)

; Extended BPB
DriveNo                 db 0                    ; Physical drive number
Reserved                db 0                    ; Reserved
BootSignature           db 029h                 ; Boot signature, indicates the presence of the following three fields
VolumeId                dd 22352E33h            ; Volume id
VolumeLabel             db 'NO NAME    '        ; Volume label
FileSystemType          db 'FAT12   '           ; File system type



;===========
; BOOTSTRAP (448-bytes)
;===========
bootstrap:
    mov     ax,07C0h
    mov     ds,ax                               ; Set data segment to where we're loaded
    mov     es,ax
    add     ax,20h                              ; Skip over the bootloader
    mov     ss,ax
    mov     sp,200h                             ; Set up a 512 bytes stack after the bootloader

    call    load_fat                            ; Loads the FAT table into memory
    call    load_root                           ; Loads the root directory into memory, and immediately searches for the kernel
    call    load_kernel                         ; Loads the kernel into memory

    mov     si,str_done
    call    print_string
    jmp     $


reboot:
    mov     si,str_error
    call    print_string
    mov     ax,0
    int     19h
    jmp     $



;===============================================
; Sets the registers necessary for loading from disk.
;===============================================
setLoadRegisters:
    push    bx
    push    ax
    mov     bx,ax                               ; Preserve the logical sector number

    mov     dx,0                                ; First sector
    div     word [SectorsPerTrack]              ; edx = eax MOD SectorsPerTrack / eax = eax DIV SectorsPerTrack
    add     dl,1                                ; Physical sectors start with 1
    mov     cl,dl                               ; int 13h uses cl for sectors

    mov     ax,bx
    mov     dx,0
    div     word [SectorsPerTrack]
    mov     dx,0
    div     word [NumberOfHeads]
    mov     dh,dl
    mov     ch,al

    pop     ax
    pop     bx
    mov     dl,byte [DriveNo]
    ret



;===============================================
; Loads the FAT table into memory.
;===============================================
load_fat:
    mov     ax,1                                ; Sector 1 is the 1st FAT table
    call    setLoadRegisters

    ; [ES:BX]
    mov     si,0800h                            ; Set the pointer to the FAT table buffer
    mov     es,si
    xor     bx,bx

    mov     ah,2                                ; I/O Read
    mov     al,9                                ; FAT table consists of 9-sectors

    stc
    int     13h                                 ; Read the sectors into the buffer
    jc      reboot                              ; An error occured
    cmp     al,9
    jne     reboot                              ; Not enough sectors were read
    ret



;===============================================
; Loads the root directory into memory.
;===============================================
load_root:
    mov     ax,19                               ; Sector 19 is the start of the root directory
    call    setLoadRegisters

    ; [ES:BX]
    mov     si,0920h                            ; Set the pointer to the buffer
    mov     es,si
    xor     bx,bx

    mov     ah,2                                ; I/O Read
    mov     al,14                               ; The root consists of 14-sectors

    stc
    int     13h                                 ; Read the sectors into memory
    jc      reboot                              ; An error occured
    cmp     al,14
    jne     reboot                              ; Not enough sectors were read
    jmp     search_kernel



;===============================================
; Search through the root directory.
;===============================================
search_kernel:
    xor     bx,bx
    mov     cx,word [MaxRootEntries]

.loop:
    xchg    cx,dx

    mov     si,kernel_name
    mov     di,bx
    mov     cx,11
    rep     cmpsb                               ; [DS:SI] [ES:DI]
    je      .found
    add     bx,32

    xchg    dx,cx
    loop    .loop

.not_found:
    jmp     reboot

.found:
    mov     ax,word [es:bx+1Ah]                 ; Logical sector ID
    ret



;===============================================
; Search through the root directory.
;===============================================
load_kernel:
    mov     bx,ax
    
    mov     bx,es
    add     bx,20h
    mov     es,bx

    ret



;===============================================
; Prints a random string to the screen.
;===============================================
print_string:
    pusha
    mov     ah,0Eh

.loop:
    lodsb
    cmp     al,0
    je      .done
    int     10h
    jmp     .loop

.done:
    popa
    ret



;===============================================
; Prints a HEX byte onto the screen.
;===============================================
print_hex:
    pusha

    mov     cx,2
    mov     bx,16                               ; Divide the input value by 16
    mov     dx,0
    div     bx

.loop:
    cmp     al,10
    jl      .low
    add     ax,07h                              ; 41h - 30h - 0Ah = 07h

.low:
    add     ax,30h
    mov     ah,0Eh
    int     10h

    mov     ax,dx
    loop    .loop

.done:
    popa
    ret



;===========
; VARIABLES
;===========
kernel_cluster          dw -1                   ; Current targeted sector to load for the kernel
kernel_name             db 'KERNEL  BIN'        ; Name of the kernel file as saved on the FAT12 volume

; TODO: strip the following
str_boot                db 'Booting',0
str_done                db 'Done',0
str_error               db 'Error',0



;===========
; BOOT SIG (2-bytes)
;===========
times 510-($-$$) db 0
sign dw 0AA55h
