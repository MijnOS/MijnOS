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
;     ds:si - The name of the file.
;     es:di - The destination to store the data.
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
; Finds the next cluster value.
;   In:
;     ax - The current logical cluster.
;   Out:
;     ax - The value of the next logical cluster.
;===============================================
fat_findNextCluster:
    push    bp
    mov     bp,sp
    sub     sp,8
    sub     sp,word [iFAT_BytesPerSector]
    sub     sp,word [iFAT_BytesPerSector]

    ; Save the destination and set the
    ; destination pointer to be our stack instead
    pusha
    push    es
    push    di
    mov     bx,ss
    mov     es,bx

    ; Immediately store the passed argument
    mov     word [bp-2],ax

;
; The input can be a variety of values. Based on
; the input we can determine if we have to load
; and which logical sector contains the data.
;
.checkInput:
    mov     ax,word [bp-2]  ; 0FF0h - Reserved
    cmp     ax,0FF0h        ; 0FF7h - Bad
    jae     .return         ; 0FF8h - Last


    ; Determine the byte the start reading from,
    ; this determines the logical sector to load
    xor     dx,dx
    mov     cx,3
    mul     cx          ; dx:ax = ax * cx
    mov     cx,2        ; dx = (ax % cx)
    div     cx          ; ax = (ax / cx)

    ; Store the values
    mov     word [bp-4],ax
    mov     word [bp-6],dx

    xor     dx,dx
    mov     cx,word [iFAT_BytesPerSector]

.calc_logical:
    cmp     ax,cx
    jae     .calc_continue
    jmp     .calc_break

.calc_continue:
    sub     ax,cx
    jmp     calc_logical

.calc_break:


.load0:




    ; Store the number of times we have to loop
    xor     dx,dx
    mov     ax,word [iFAT_SectorsPerFat]
    mov     cx,3
    div     cx
    mov     word [bp-4],ax      ; max
    mov     word [bp-6],0       ; current

    ; Set the destination to the 3x sectors reserved on the stack
    mov     dx,word [iFAT_BytesPerSector]
    shl     dx,1
    add     dx,word [iFAT_BytesPerSector]
    lea     ax,[bp]
    sub     ax,8
    sub     ax,dx
    mov     di,ax


.loadNextGroup:
    mov     ax,word [bp-4]
    mov     word [bp-8],ax

.load_loop:
    mov     ax,word [bp-6]
    call    fat_loadFatSector
    add     word [bp-6],1
    sub     word [bp-8],1
    cmp     word [bp-8],0
    ja      .load_loop


.return:
    pop     di
    pop     es
    popa
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Searches for and copies the found entry.
;   In:
;     ds:si - The name of the entry.
;     es:di - The destination to copy the entry
;             information to. (sizeof == 32)
;   Out:
;     ax - Zero if found; otherwise, non-zero.
;===============================================
fat_searchEntry:
    push    bp
    mov     bp,sp
    sub     sp,8
    sub     sp,word [iFAT_BytesPerSector]
    pusha
    push    es                                  ; Save the destination
    push    di
    mov     bx,ss
    mov     es,bx

    mov     ax,word [iFAT_BytesPerSector]
    shr     ax,5
    mov     word [bp-6],ax                      ; WORD entriesPerSector
    
    call    fat_getSizeRoot
    mov     word [bp-2],ax                      ; WORD rootSize
    mov     word [bp-4],0                       ; WORD rootIndex

.loadSector:
    mov     word [bp-8],0                       ; WORD entriesIndex

    lea     ax,[bp]
    sub     ax,word [iFAT_BytesPerSector]
    sub     ax,8
    mov     di,ax                               ; void * sector // Location to store the read sector

    mov     ax,word [bp-4]
    call    fat_loadRootSector
    test    ax,ax
    jne     .error

    lea     dx,[bp]
    sub     dx,word [iFAT_BytesPerSector]
    sub     dx,8                                ; dx now points to the first entry of the loaded sector

.search:
    mov     cx,11                               ; name + extension
    mov     di,dx
    call    fat_strcmp
    test    ax,ax
    je      .copy

    add     dx,32                               ; 32-bytes per entry

    mov     ax,word [bp-8]
    add     ax,1
    cmp     ax,word [bp-6]
    mov     word [bp-8],ax                      ; entriesIndex < entriesPerSector
    jb      .search

    mov     ax,word [bp-4]
    add     ax,1
    cmp     ax,word [bp-2]                      ; rootIndex < rootSize
    mov     word [bp-4],ax
    jb      .loadSector

.error:
    pop     di
    pop     es
    popa
    mov     ax,1                                ; return 1
    mov     sp,bp
    pop     bp
    ret

.copy:
    mov     ax,di

    pop     di          ; Restore the original destination
    pop     es

    push    si
    mov     si,ax       ; start of entry
    mov     cx,32       ; 32-bytes
    
    push    ds
    push    bx
    mov     bx,ss
    mov     ds,bx
    rep movsb
    pop     bx
    pop     ds

    pop     si

.return:
    
    popa
    xor     ax,ax                               ; return 0
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Loads a FAT root or table sector
;   In:
;     ax - The offset within the root sector. [0, 14)
;     es:di - The destination to load the sector to.
;   Out:
;     ax - Zero if successful.
;===============================================
fat_loadRootSector:
    pusha
    mov     cx,ax
    call    fat_getLogicalRoot
    jmp     load_generic

fat_loadFatSector:
    pusha
    mov     cx,ax
    call    fat_getLogicalFat
    jmp     load_generic

load_generic:
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
;     ds:si - char * src
;     es:di - char * dest
;   Out:
;     N/A
;===============================================
fat_sanitize:
    push    si
    push    di

.loop:
    mov     al,byte [ds:si]
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
    mov     byte [es:di],al
    inc     di

.continue:
    inc     si
    jmp     .loop

.return:
    mov     byte [es:di],0                      ; Always appaend a string-terminator
    pop     di
    pop     si
    ret


;===============================================
; Compares two FAT name compliant strings.
;   In:
;     ds:si - char * str0
;     es:di - char * str1
;     cx - short num
;   Out:
;     ax - Zero if a match; otherwise, one.
;===============================================
fat_strcmp:
    push    di
    push    si
    push    cx

.loop:
    dec     cx

    mov     al,byte [ds:si]
    cmp     al,byte [es:di]     ; *s0 != *s1    ; <NOT EQUAL>
    jne     .mismatch

    cmp     al,0                ; *s0 == '\0'   ; <EQUAL> string terminator
    je      .match

    cmp     al,20h              ; *s0 == ' '    ; <EQUAL> end-of-string
    je      .match

    inc     di
    inc     si
    cmp     cx,0
    jne     .loop
    ;jmp    .match                              ; <EQUAL> no special characters

.match:
    xor     ax,ax
    jmp     .return

.mismatch:
    mov     ax,1
    
.return:
    pop     cx
    pop     si
    pop     di
    ret


;===============================================
; Returns a pointer to the right most occurence
; of the matching character within the string.
;   In:
;     ds:si - char * str
;     al - char character
;   Out:
;     ax - NULL if no match; otherwise, char*
;===============================================
fat_strrchr:
    push    si
    push    di
    xor     di,di

.loop:
    cmp     al,byte [ds:si]
    je      .match
    cmp     byte [ds:si],0
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
    push    dx
    movzx   ax,byte [iFAT_NumberOfFats]
    imul    ax,word [iFAT_SectorsPerFat]
    pop     dx
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


fat_getStartFat:                                ; short fat_getStartFat( short idxTable )
    push    dx
    call    fat_getLogicalFat
    imul    ax,word [iFAT_BytesPerSector]
    ;imul    ax,word [iFAT_SectorsPerCluster]
    pop     dx
    ret


fat_getStartRoot:                               ; short fat_getStartRoot( void )
    push    dx
    call    fat_getLogicalRoot
    imul    ax,word [iFAT_BytesPerSector]
    ;imul    ax,word [iFAT_SectorsPerCluster]
    pop     dx
    ret


fat_getStartData:                               ; short fat_getStartData( void )
    push    dx
    call    fat_getLogicalData
    imul    ax,word [iFAT_BytesPerSector]
    ;imul    ax,word [iFAT_SectorsPerCluster]
    pop     dx
    ret
