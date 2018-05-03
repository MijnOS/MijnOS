[BITS 16]


;===========
; ERROR CODES
;===========
%define FERR_LOAD_FILE_0            0FFFFh      ; -1
%define FERR_LOAD_FILE_1            0FFFEh      ; -2
%define FERR_ENTRY_BY_ID            0FFFDh      ; -3
%define FERR_LOAD_GENERIC_0         0FFFCh      ; -4
%define FERR_LOAD_GENERIC_1         0FFFBh      ; -5


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
; Writes a file to the medium.
;   In:
;     ?
;   Out:
;     ax - Zero if successful.
;===============================================
fat_writeFile:
    push    bp
    mov     bp,sp

    mov     sp,bp
    pop     bp
    ret



; Searches for the first empty cluster in the
; FAT table.
fat_findEmptyCluster:
    push    bp
    mov     bp,sp
    sub     sp,8
    sub     sp,word [iFAT_BytesPerSector]
    sub     sp,word [iFAT_BytesPerSector]

.loadNext:
    lea     bx,[bp]
    sub     bx,8
    sub     bx,word [iFAT_BytesPerSector]
    sub     bx,word [iFAT_BytesPerSector]
    mov     di,bx

    call    fat_loadFatSector                   ; 1st sector
    test    ax,ax
    jne     .return 

    ; TODO: load next sector
    add     ax,1
    add     di,word [iFAT_BytesPerSector]

    call    fat_loadFatSector                   ; 2nd sector
    test    ax,ax
    jne     .return

    ; TODO: load value into cx

.odd:
    shr     cx,4
    jmp     .compare

.even:
    and     cx,0FFFh
    jmp     .compare

.compare:
    test    cx,cx
    jne     .compareNext            ; TODO: actual next

.return:
    mov     sp,bp
    pop     bp
    ret


; Searches for and returns the index of the
; first empty file entry in the root directory.
fat_findEmptyFileEntry:
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
    mov     di,ax

    mov     ax,word [bp-4]
    call    fat_loadRootSector
    test    ax,ax
    jne     .error

    lea     dx,[bp]
    sub     dx,word [iFAT_BytesPerSector]
    sub     dx,8                                ; dx now points to the first entry of the loaded sector

.search:
    mov     di,dx
    mov     al,byte [es:di]
    cmp     al,0E5h     ; State: erased
    je      .calculate
    cmp     al,0        ; State: free
    je      .calculate

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
    mov     ax,0FFFFh
    mov     sp,bp
    pop     bp
    ret

.calculate:
    xor     dx,dx
    mov     ax,word [bp-4]      ; bp-4 = rootIndex
    mov     cx,word [bp-6]      ; bp-6 = entriesPerSector
    mul     cx
    mov     cx,word [bp-8]      ; bp-8 = entriesIndex
    add     ax,cx
    mov     word [bp-2],ax
    ; entriesPerSector * rootIndex
    ; + entriesIndex

.return:
    pop     di
    pop     es
    popa
    mov     ax,word [bp-2]
    mov     sp,bp
    pop     bp
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
    sub     sp,44                               ; FAT name + extension + FAT entry
    sub     sp,MAX_PATH                         ; Input path
    pusha
    ;push    ds
    push    es
    push    di

    mov     bx,ss                               ; Copy to the stack
    mov     es,bx

.copy_start:
    ;lea     di,[bp-MAX_PATH-44]                 ; Copies the string from the source onto the stack
    ;mov     cx,MAX_PATH
    ;call    strncpy

    ;mov     bx,ss
    ;mov     ds,bx

.get_entry:
    ;lea     si,[bp-MAX_PATH-44]                 ; TODO: ERROR: DOES NOT SEEM TO RETURN!
    lea     di,[bp-44]
    call    fat_searchEntry
    test    ax,ax
    jne     .error0

    ; NOTE: good
    ;mov     ax,[bp-18]
    ;call    print_hex

    pop     di          ; We need our original destination
    pop     es
    ;pop     ds

    mov     bx,di               ; Buffer pointer
    mov     ax,word [bp-18]     ; Logical sector

.load_loop:
    call    fat_logicalToPhysical
    call    fat_setLoadRegisters

    mov     ah,2
    mov     al,1
    
    stc
    int     13h
    jc      .error1
    cmp     al,1
    jne     .error2
    
    add     bx,word [iFAT_BytesPerSector]

    mov     ax,word [bp-18]
    call    fat_findNextCluster
    mov     word [bp-18],ax

    cmp     ax,0FF0h
    jb      .load_loop

    mov     word [bp-2],0
    jmp     .return

.error0:
    pop     di
    pop     es
    ;pop     ds
    mov     word [bp-2],ax
    jmp     .return

.error1:
    mov     word [bp-2],FERR_LOAD_FILE_0
    jmp     .return

.error2:
    mov     word [bp-2],FERR_LOAD_FILE_1
    jmp     .return

.return:
    popa
    mov     ax,word [bp-2]
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Gets the size of the file.
;   In:
;     ds:si - The name of the entry.
;   Out:
;     dx:ax - The size of the file, -1 on error.
;===============================================
fat_getFileSize:
    push    bp
    mov     bp,sp
    sub     sp,32
    pusha
    push    es
        
.load_entry:
    mov     bx,ss
    mov     es,bx
    lea     di,[bp-32]
    call    fat_searchEntry
    test    ax,ax
    jne     .return

.error:
    mov     word [bp-4],0FFFFh
    mov     word [bp-2],0FFFFh

.return:
    pop     es
    popa
    mov     dx,word [bp-4]
    mov     ax,word [bp-2]
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

    ;cmp     ax,word [iFAT_SmallSectors]
    ;jb      .validArg
    ;mov     sp,bp
    ;pop     bp
    ;ret

.validArg:
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
    mov     ax,word [bp-2]                      ; 0FF0h - Reserved
    cmp     ax,0FF0h                            ; 0FF7h - Bad
    jae     .return                             ; 0FF8h - Last


    ; Determine the byte the start reading from,
    ; this determines the logical sector to load
    xor     dx,dx
    mov     cx,3
    mul     cx                                  ; dx:ax = ax * cx
    mov     cx,2                                ; dx = (ax % cx)
    div     cx                                  ; ax = (ax / cx)

    ; Store the values
    mov     word [bp-4],ax                      ; byte offset
    mov     word [bp-6],dx                      ; modulo


    ; ax determines the logical sector
    xor     dx,dx
    mov     cx,word [iFAT_BytesPerSector]
    div     cx
    mov     word [bp-8],ax                      ; logical sector

    ; Destination to read to
    lea     bx,[bp]
    sub     bx,8
    sub     bx,word [iFAT_BytesPerSector]
    sub     bx,word [iFAT_BytesPerSector]
    mov     di,bx

    call    fat_loadFatSector                   ; 1st sector
    test    ax,ax
    jne     .return

    mov     ax,word [bp-8]
    add     ax,1
    add     di,word [iFAT_BytesPerSector]

    call    fat_loadFatSector                   ; 2nd sector
    test    ax,ax
    jne     .return

    ; Determine the offset within our buffer
    mov     ax,word [bp-8]                      ; logical sector
    shl     ax,9                                ; *512
    mov     dx,word [bp-4]                      ; byte offset
    sub     dx,ax                               ; offset within our buffer

    ; Take the value of the two bytes on our stack
    ;sub     bx,word [iFAT_BytesPerSector]
    add     bx,dx                               ; Points to our buffer
    mov     cx,word [ss:bx]                     ; This is the value we seek

    mov     ax,word [bp-6]
    test    ax,ax
    je      .even

.odd:
    shr     cx,4
    jmp     .result

.even:
    and     cx,0FFFh
    jmp     .result

.result:
    mov     word [ebp-2],cx
    pop     di
    pop     es
    popa
    mov     ax,word [ebp-2]
    mov     sp,bp
    pop     bp
    ret  

.return:
    pop     di
    pop     es
    popa
    mov     sp,bp
    pop     bp
    ret



;===============================================
; Searches for the FAT entry by its id.
;   In:
;     ax - The id of the entry to find.
;     es:di - The destination to copy the entry
;             information to. (sizeof == 32)
;   Out:
;     ax - Zero if found; otherwise, non-zero.
;===============================================
fat_getEntryById:
    push    bp
    mov     bp,sp
    sub     sp,8
    sub     sp,word [iFAT_BytesPerSector]
    pusha
    push    si
    push    es                                  ; Original destination
    push    di

    cmp     ax,0
    jl      .error
    cmp     ax,word [iFAT_MaxRootEntries]
    jge     .error

    mov     bx,ss
    mov     es,bx

    mov     cx,ax

    mov     ax,word [iFAT_BytesPerSector]
    shr     ax,5
    mov     word [bp-6],ax                      ; WORD entriesPerSector

    xchg    ax,cx
    xor     dx,dx
    idiv    cx

    mov     word [bp-2],dx                      ; WORD offset
    mov     word [bp-4],ax                      ; WORD logicalSector

.loadSector:
    lea     cx,[bp]
    sub     cx,word [iFAT_BytesPerSector]
    sub     cx,8
    mov     di,cx

    call    fat_loadRootSector
    test    ax,ax
    jne     .error

.copy:
    mov     ax,word [bp-2]
    shl     ax,5
    mov     si,ax
    add     si,di       ; Points to the requested entry

    pop     di
    pop     es

    push    ds
    push    bx
    mov     bx,ss
    mov     ds,bx
    mov     cx,32       ; [es:di] = dest
    rep movsb           ; [ds:si] = src
    pop     bx
    pop     ds

    pop     si
    popa
    xor     ax,ax
    mov     sp,bp
    pop     bp
    ret

.error:
    pop     di
    pop     es
    pop     si
    popa
    mov     ax,FERR_ENTRY_BY_ID
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
    mov     word [bp-2],ax
    pop     di
    pop     es
    popa
    mov     ax,word [bp-2]
    ;mov     ax,1                                ; return 1
    mov     sp,bp
    pop     bp
    ret

.copy:
    mov     ax,di

    pop     di                                  ; Restore the original destination
    pop     es

    push    si
    mov     si,ax                               ; start of entry
    mov     cx,32                               ; 32-bytes
    
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
    xor     ax,ax
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
    jc      .error0                             ; Hardware error
    cmp     al,1                                ; Insufficient number of sectors read
    jne     .error1
    ;jmp     .return

.return:
    popa
    xor     ax,ax
    ret

.error0:
    popa
    mov     ax,FERR_LOAD_GENERIC_0
    ret

.error1:
    popa
    mov     ax,FERR_LOAD_GENERIC_1
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


fat_logicalToPhysical:
    push    cx
    mov     cx,ax
    call    fat_getLogicalData
    sub     ax,2
    add     ax,cx
    pop     cx
    ret


;===========
; FAT error messages
;===========
ferr_load0  db 'No matching name', 0
ferr_load1  db 'Load error', 0
ferr_load2  db 'Sector error', 0

ferr_fsize  db 'File size', 0

ferr_fread0 db 'I/O Error', 0
ferr_fread1 db 'I/O Sector', 0



