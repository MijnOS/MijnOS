[BITS 16]

;===========
; Information needed for FAT12 I/O
;===========
iFAT_BytesPerSector     dw 512                  ; Number of bytes per sector
iFAT_SectorsPerCluster  db 1                    ; Number of sectors per cluster
iFAT_ReservedSectors    dw 1                    ; Number of reserved sectors
iFAT_NumberOfFats       db 2                    ; Number of FAT tables
iFAT_MaxRootEntries     dw 224                  ; Maximum number of root directories
iFAT_SmallSectors       dw 2880                 ; Total sector count (For FAT16 and older)
iFAT_SectorsPerFat      dw 9                    ; Sectors per FAT
iFAT_SectorsPerTrack    dw 18                   ; Sectors per track
iFAT_NumberOfHeads      dw 2                    ; Number of heads
iFAT_HiddenSectors      dd 0                    ; Number of hidden sectors
iFAT_DriveNo            db 0                    ; Physical drive number

;===========
; VARIABLES
;===========
fat_buffer              times 512 db 0          ; 512-bytes so we can read a single cluster
.length                 equ ($-fat_buffer)


;===============================================
; Sets the registers necessary for loading from disk.
;   In:
;     ax - The logical sector number.
;   Out:
;     ax, ch, cl, dh, cl
;===============================================
fat_setLoadRegisters:
    push    bx
    push    ax
    mov     bx,ax                               ; Preserve the logical sector number

    mov     dx,0                                ; First sector
    div     word [iFAT_SectorsPerTrack]         ; edx = eax MOD SectorsPerTrack / eax = eax DIV SectorsPerTrack
    add     dl,1                                ; Physical sectors start at 1 not 0
    mov     cl,dl                               ; int 13h uses cl for sectors

    mov     ax,bx
    mov     dx,0
    div     word [iFAT_SectorsPerTrack]
    mov     dx,0
    div     word [iFAT_NumberOfHeads]
    mov     dh,dl
    mov     ch,al

    pop     ax
    pop     bx
    mov     dl,byte [iFAT_DriveNo]
    ret


;===============================================
; Loads a file from the FAT and stores it in the
; destination. It assumes the destination is
; large enough to hold the file.
;   In:
;     si - The name of the file.
;     di - The destination to store the data.
;   Out:
;     ax - Zero if successful.
;===============================================
fat_loadFile:
    push    bp
    mov     bp,sp
    sub     sp,4
    pusha
    jmp     .return     ; TODO: This function is not yet finished

    call    fat_getSizeRoot                     ; short max
    mov     word [bp-4],ax

    xor     cx,cx                               ; short index
    mov     word [bp-2],cx

.loadNextRoot:                                  ; 1) Load a part of the FAT root

    call    fat_getLogicalRoot
    add     ax,word [bp-2]
    call    fat_setLoadRegisters

    ;mov     es,si      ; NOTE: Should already be set properly!
    mov     bx,fat_buffer

    mov     ah,2                                ; I/O Read
    mov     al,1                                ; Only 1 sector every turn
    
    stc
    int     13h                                 ; Read a sector of the root
    jc      .error                              ; Hardware error
    cmp     al,1                                ; Insufficient number of sectors read
    jne     .error

    ; NOTE:
    ;   At this point the sector has been read,
    ;   so we now need to search for the file.
    mov     cx,word [iFAT_BytesPerSector]
    shr     cx,5                                ; Entries per sector


.findMatch:
    push    cx
    push    si
    push    di

    mov     di,cx
    sub     di,1
    imul    di,32


    mov     cx,11       ; Compare both name and extension
    rep     cmpsb
    je      .found
    
    pop     di
    pop     si
    pop     cx
    loop    .findMatch

.continue:
    add     word [bp-2],1
    mov     ax,word [bp-2]                      ; ++index < max
    cmp     ax,word [bp-4]
    jb      .notFound

.found:
    jmp     .return


.notFound:                                      ; x) File not found
    jmp     .return

.error:
    jmp     .return

.return:
    popa
    mov     sp,bp
    pop     bp
    ret


;===============================================
; N/A
;   In:
;     si - The name of the entry.
;     di - The destination to copy the entry
;          information to. (sizeof == 32)
;   Out:
;     ax - Zero if found; otherwise, non-zero.
;===============================================
fat_searchEntry:
    ret



;===============================================
; Loads a FAT root sector
;   In:
;     ax - The offset within the root sector. [0, 14)
;     di - The destination to load the sector to.
;   Out:
;     ax - Zero if successful.
;===============================================
fat_loadRootSector:
    pusha

    mov     cx,ax
    call    fat_getLogicalRoot
    add     ax,cx
    call    fat_setLoadRegisters

    ;mov     es,si                               NOTE: Should already be set properly!
    mov     bx,di

    mov     ah,2                                ; I/O Read
    mov     al,1                                ; Only 1 sector
    
    stc
    int     13h                                 ; I/O Read
    jc      .error                              ; Hardware error
    cmp     al,1                                ; Insufficient number of sectors read
    jne     .error
    ;jmp     .return

.return:
    popa
    xor     ax,ax
    ret

.error:
    popa
    mov     ax,1
    ret


;===============================================
; Converts the input string into a valid FAT
; string, consisting of only letters and digits.
;   In:
;     si - char * src
;     di - char * dest
;   Out:
;     N/A
;===============================================
fat_sanitize:
    push    si
    push    di

.loop:
    mov     al,byte [si]
    cmp     al,0                                ; string-terminator
    je      .return
    
.stage1:                                        ; lowercase to uppercase
    cmp     al,61h
    jb      .stage2
    cmp     al,7Ah
    ja      .stage2
    sub     al,20h
    jmp     .valid

.stage2:                                        ; A-Z
    cmp     al,41h
    jb      .stage3
    cmp     al,5Ah
    jbe     .valid

.stage3:                                        ; 0-9
    cmp     al,30h
    jb      .continue
    cmp     al,39h
    ja      .continue
    ;jmp     .valid

.valid:
    mov     byte[di],al

.continue:
    inc     di
    inc     si
    jmp     .loop

.return:
    mov     byte [di],0                         ; Always appaend a string-terminator
    pop     di
    pop     si
    ret


;===============================================
; Compares two FAT name compliant strings.
;   In:
;     si - char * str0
;     di - char * str1
;     cx - short num
;   Out:
;     ax - Zero if a match; otherwise, one.
;===============================================
fat_strcmp:
    push    bx
    mov     bx,cx

.loop:
    dec     bx

    mov     al,byte [si+bx]
    cmp     al,byte [di+bx] ; *s0 != *s1        ; <NOT EQUAL>
    jne     .mismatch

    cmp     al,0            ; *s0 == '\0'       ; <EQUAL> string terminator
    je      .match

    cmp     al,20h          ; *s0 == ' '        ; <EQUAL> end-of-string
    je      .match

    cmp     bx,0
    jne     .loop
    ;jmp    .match                              ; <EQUAL> no special characters

.match:
    xor     ax,ax
    jmp     .return

.mismatch:
    mov     ax,1
    
.return:
    pop     bx
    ret



;===============================================
; Returns a pointer to the right most occurence
; of the matching character within the string.
;   In:
;     si - char * str
;     al - char character
;   Out:
;     ax - NULL if no match; otherwise, char*
;===============================================
fat_strrchr:
    push    si
    push    di
    xor     di,di

.loop:
    cmp     al,byte [si]
    je      .match
    cmp     byte [si],0
    je      .return
    add     si,1
    jmp     .loop

.match:
    mov     di,si
    jmp     .loop

.return:
    mov     ax,di
    pop     di
    pop     si
    ret




;===============================================
; UTILITY
;   All the utility functions preserve all
;   registers except for the return register ax.
;===============================================
fat_getSizeFats:                                ; short getSizeFats( void )
    movzx   ax,byte [iFAT_NumberOfFats]
    imul    ax,word [iFAT_SectorsPerFat]
    ret


fat_getSizeRoot:                                ; short getSizeRoot( void )
    push    dx
    mov     ax,word [iFAT_MaxRootEntries]
    shl     ax,5                                ; 32-bytes per entry
    xor     dx,dx
    div     word [iFAT_BytesPerSector]
    pop     dx
    ret


fat_getLogicalFat:                              ; short getLogicalFat( short idxTable )
    imul    ax,word [iFAT_SectorsPerFat]
    add     ax,word [iFAT_ReservedSectors]
    ret


fat_getLogicalRoot:                             ; short getLogicalRoot( void )
    call    fat_getSizeFats
    add     ax,word [iFAT_ReservedSectors]
    ret


fat_getLogicalData:                             ; short getLogicalData( void )
    push    bx
    mov     bx,word [iFAT_ReservedSectors]
    call    fat_getSizeFats
    add     bx,ax
    call    fat_getSizeRoot         
    add     ax,bx
    pop     bx
    ret

