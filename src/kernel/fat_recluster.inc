;===============================================
; Copies data from n-bytes from [ds:si] to the
; stack.
;===============================================
%macro mCopyToStack 2

    push    bx
    push    cx
    push    si
    push    di
    push    es

    mov     bx,ss
    mov     es,bx
    lea     di,[bp-%1]
    mov     cx,%2

    rep movsb

    pop     es
    pop     di
    pop     si
    pop     cx
    pop     bx

%endmacro

;===============================================
; Searches for the matching file entry; if found
; it will be copied to [ss:bp-%1]
;===============================================
%macro mSearchEntry 1

    push    es
    push    bx
    push    di

    mov     bx,ss
    mov     es,bx
    lea     di,[bp-%1]
    call    fat_searchEntry

    pop     di
    pop     bx
    pop     es

%endmacro


;===============================================
; Determines the current number of used clusters.
;   In:
;     ax - The starting cluster.
;   Out:
;     ax - The number of clusters.
;===============================================
fat_calcClusters:
    push    cx
    xor     cx,cx

; loop till no more clusters are linked
.loop:
    test    ax,ax       ; 0 = unused
    jbe     .return     ; >= 0FF0h reserved
    cmp     ax,0FF0h    ; == 0FF7h bad
    jae     .return     ; >= 0FF8h last-of-file
    add     cx,1
    call    fat_getClusterValue
    jmp     .loop

; return to the caller
.return:
    mov     ax,cx
    pop     cx
    ret
; TEST: OK


;===============================================
; Gets the index of the cluster for the file.
;   In:
;     ax - The starting cluster.
;   Out:
;     ax - Zero if failed; otherwise, the cluster index.
;===============================================
fat_getLastFileCluster:
    push    cx
    push    dx
    xor     cx,cx
    xor     dx,dx

; loop till no more clusters are linked
.loop:
    test    ax,ax       ; < 0 = program error
    jbe     .return     ; 0 = unused
    cmp     ax,0FF0h    ; >= 0FF0h reserved
    jae     .return     ; == 0FF7h bad
    mov     dx,ax       ; >= 0FF8h last-of-file
    call    fat_getClusterValue
    jmp     .loop

; return to the caller
.return:
    mov     ax,dx
    pop     dx
    pop     cx
    ret
; TEST: OK


;===============================================
; Allocates and links the specified number of
; clusters.
;   In:
;     ax - The starting cluster.
;     cx - The number of clusters to allocate.
;   Out:
;     ax - Zero if successful.
;     cx - Starting cluster.
;===============================================
fat_allocClusters:
    push    bp
    mov     bp,sp
    sub     sp,6

; if -1 we want to allocate from the beginning
; up to the end of the file.
.params:
    cmp     ax,0FFFFh                           ; the cluster to start with has
    jne     .start                              ; already been allocated

; we need to allocate the first cluster
.first:
    call    fat_findEmptyCluster                ; find first cluster for file
    cmp     ax,0FF0h
    jae     .error                              ; ensure error free
  
    mov     word [bp-4],ax

    push    cx
    mov     cx,0FFFh
    call    fat_setClusterValue                 ; set the first cluster to be last-of-file
    pop     cx

    test    ax,ax
    jne     .error                              ; ensure error free

    mov     ax,word [bp-4]
    ;sub     cx,1                                ; we have allocated a cluster

.start:
    mov     word [bp-6],ax

; keep looping till no more clusters have to be
; allocted.
.loop:
    mov     word [bp-2],ax                      ; store the current cluster

    push    cx
    mov     cx,0FFFh
    call    fat_setClusterValue                 ; set the initial state to -1
    pop     cx

    test    ax,ax
    jne     .error                              ; ensure error free

    call    fat_findEmptyCluster                ; find the next empty cluster
    cmp     ax,0FF0h
    jae     .error                              ; ensure error free

    mov     word [bp-4],ax                      ; store the next cluster

    push    cx
    mov     cx,ax                               ; the new value is the new cluster id
    mov     ax,word [bp-2]                      ; current cluster
    call    fat_setClusterValue                 ; set the cluster behind the current cluster to be the new cluster
    pop     cx

    test    ax,ax
    jne     .error                              ; ensure error free set

    mov     ax,word [bp-4]                      ; the next cluster will become the current
    loop    .loop

; set the last cluster to end-of-file
.last:
    mov     ax,word [bp-2]
    mov     cx,0FFFh
    call    fat_setClusterValue

    test    ax,ax
    jne     .error                              ; ensure error free

; when it reaches here, the op was succesful
.success:
    xor     ax,ax

; return to the caller
.return:
    mov     cx,word [bp-6]
    mov     sp,bp
    pop     bp
    ret

; called when an error occurs
.error:
    mov     ax,0FFFFh
    jmp     .return
; TEST: OK


;===============================================
; Frees all the clusters behind the current
; cluster.
;   In:
;     ax - The starting cluster.
;   Out:
;     ax - Zero if successful.
;===============================================
fat_freeClusters:
    push    bp
    mov     bp,sp
    sub     sp,4

; store the initial values
.init:
    test    ax,ax
    je      .return
    cmp     ax,0FF0h
    jae     .return                 ; ensure valid cluster id
    mov     word [bp-2],ax

; first set the cluster value of the new last of
; file to be the actual last-of-file.
.first:
    call    fat_getClusterValue     ; get the id of the next cluster
    mov     word [bp-4],ax          ; store the next cluster id

    mov     ax,word [bp-2]          ; current cluster id
    mov     cx,0FFFh                ; last-of-file
    call    fat_setClusterValue     ; change to last-of-file

    test    ax,ax
    jne     .return                 ; ensure error free

; keep looping till no more cluster can be freed.
.loop:
    mov     ax,word [bp-4]          ; next cluster
    test    ax,ax                   ; unused/free
    je      .return
    cmp     ax,0FF8h                ; error state(s)
    jae     .return
    cmp     ax,0FF0h                ; last-of-file
    jae     .last
    mov     word [bp-2],ax          ; current = next

; this cluster has another behind it
.intermediate:
    call    fat_getClusterValue
    mov     word [bp-4],ax          ; store the next cluster id 

    mov     ax,word [bp-2]
    mov     cx,0
    call    fat_setClusterValue     ; set the cluster to free

    test    ax,ax
    je      .loop                   ; ensure error free

; return to the caller
.return:
    mov     sp,bp
    pop     bp
    ret

; last cluster of the file
.last:
    mov     ax,word [bp-2]
    mov     cx,0
    call    fat_setClusterValue
    ;test    ax,ax
    ;just return the value
    xor     ax,ax
    jmp     .return
; TEST: OK



;===============================================
; Reallocates clusters. This includes growning
; as well as shrinking in number of clusters.
;   In:
;     ax - The number of necessary clusters.
;     cx - The starting cluster.
;   Out:
;     ax - Zero if successful.
;===============================================
fat_relocClusters2:
    push    bp
    mov     bp,sp
    sub     sp,8
    pusha

    ; default return value
    mov     word [bp-8],0FFFFh

; parameters check
.params:
    cmp     ax,500
    ja      .return     ; up to 500 clusters not any larger
    cmp     ax,0
    jbe     .return     ; zero or negative clusters is not allowed either

    ;cmp     cx,33       ; BOOT + 2x FAT + ROOT
    cmp     cx,1        ; volume entries
    jbe     .return     ; first are special
    cmp     cx,0FF0h
    jb      .init       ; valid cluster id
    cmp     cx,0FF8h
    jb      .init       ; reserved/bad cluster

; initialize for use
.init:
    mov     word [bp-2],ax  ; reqClusters - The requested number of clusters.
    mov     word [bp-4],cx  ; rootCluster - The starting cluster.
    mov     word [bp-6],0   ; numClusters - The number of used clusters.

; get the current length
.length:
    mov     ax,word [bp-4]
    call    fat_calcClusters
    mov     word [bp-6],ax

; determine the difference between the currently
; number of used and requested number of clusters.
.difference:
    mov     ax,word [bp-2]  ; reqClusters
    mov     cx,word [bp-6]  ; numClusters
    cmp     ax,cx
    jb      .lower      ; reqClusters < numClusters
    cmp     ax,cx
    ja      .higher     ; reqClusters > numClusters
    jmp     .equal      ; reqClusters == numClusters

; the requested and currently used match up
.equal:
    mov     word [bp-8],0   ; success
    jmp     .return

; an x-amount of clusters can be freed; skip for now
.lower:
    ; TODO: if time/for future builds
    mov     word [bp-8],0   ; success
    jmp     .return

; we need to allocate more clusters
.higher:
    sub     ax,cx       ; diff = ( reqClusters - numClusters )
    push    ax
    mov     ax,word [bp-4]
    call    fat_getLastFileCluster
    ;mov     dx,ax       ; last cluster id
    pop     cx          ; diff (aka num to allocate)

    test    ax,ax
    je      .error

    call    fat_allocClusters
    test    ax,ax
    jne     .error

; when here it is successful
.success:
    mov     word [bp-8],0

; return to the caller
.return:
    popa
    mov     ax,word [bp-8]
    mov     sp,bp
    pop     bp
    ret
    
; an error occured
.error:
    mov     word [bp-8],0FFFFh
    jmp     .return
; TEST: OK


;===============================================
; Creates a file entry.
;   In:
;     ds:si - File name
;   Out:
;     ax - Zero if successful.
;===============================================
fat_createFile:
    push    bp
    mov     bp,sp
    sub     sp,48
    push    bx
    push    cx
    push    dx
    push    si
    push    di

; searches for an empty file entry
.findEmptyEntry:
    call    fat_findEmptyFileEntry
    test    ax,0FFFFh
    je      .error
    mov     word [bp-46],ax

; find an empty cluster
.findEmptyCluster:
    call    fat_findEmptyCluster
    cmp     ax,07FFFh
    jge     .error
    mov     word [bp-48],ax

; clears the stack entry
.clearStackEntry:
    push    di
    push    es
    mov     bx,ss
    mov     es,bx
    lea     di,[bp-44]
    mov     cx,32
    xor     ax,ax 
    rep stosb
    pop     es
    pop     di

; fills the entry with proper data; NOTE that
; everything is set to zero, as such we only
; need to fill the required fields
.fillStackEntry:

; copies the name
.copy_name:
    mCopyToStack 44, 11         ; 11 = name + extension

; sets the entry as archive
.set_archive:
    push    es
    mov     bx,ss
    mov     es,bx
    lea     di,[bp-44]
    add     di,11
    mov     byte [es:di],020h
    pop     es

; reserved the cluster on the disk
.reserve_firstCluster:
    mov     ax,word [bp-48]
    mov     cx,0FFFh
    call    fat_setClusterValue
    cmp     ax,ax
    jne     .error

; set the file entry's cluster value
.set_cluster:
    push    es
    mov     bx,ss
    mov     es,bx
    lea     di,[bp-44]
    add     di,26               ; first logical cluster
    mov     ax,word [bp-48]
    mov     word [es:di],ax
    pop     es

; sets the filesize to a single cluster
.set_filesize:
    push    es
    mov     bx,ss
    mov     es,bx
    lea     di,[bp-44]
    add     di,28
    mov     ax,word [gs:iFAT_BytesPerSector]
    mov     word [es:di],ax         ; single cluster
    mov     word [es:di+2],0        ; upper 16-bits
    pop     es

; write to the drive
.write_to_drive:
    push    es
    mov     bx,ss
    mov     es,bx
    mov     ax,word [bp-46]         ; index
    lea     si,[bp-44]
    call    fat_rootSetEntry        ; Write the entry to the disk
    pop     es

    test    ax,ax
    jne     .error

; success/no errors
.success:
    xor     ax,ax

; return to the caller
.return:
    pop     di
    pop     si
    pop     dx
    pop     cx
    pop     bx
    mov     sp,bp
    pop     bp
    ret

; an error occured
.error:
    mov     ax,0FFFFh
    jmp     .return
; 0-7   BYTE filename
; 8-10  BYTE extension
; 11    BYTE attributes
; 12-13 WORD reserved
; 14-15 WORD creationTime
; 16-17 WORD creationDate
; 18-19 WORD lastAccessDate
; 20-21 WORD zeros
; 22-23 WORD lastWriteTime
; 24-25 WORD lastWriteDate
; 26-27 WORD firstLogicalCluster
; 28-31 DWORD fileSize



;===============================================
; Reads a file entry from the medium. Creates
; an entry if it does not yet exist.
;   In:
;     ds:si - File name
;     es:di - Destination to copy to
;   Out:
;     ax - Zero if successful.
;===============================================
fat_fileReadOrCreateEntry:
    push    bp
    mov     bp,sp
    sub     sp,44
    push    bx

; searches for an existing entry.
.findExisting:
    mSearchEntry 44
    test    ax,ax       ; entry does exist
    je      .copy

; file does not exist, create it
.fileNotFound:
    call    fat_createFile
    cmp     ax,ax
    jne     .return

; try searching for the new entry
.createSearch:
    mSearchEntry 44
    test    ax,ax       ; entry does exist
    je      .copy

; copy from stack to destination
.copy:
    push    ds
    push    si
    push    bx
    push    cx

    mov     bx,ss       ; source
    mov     ds,bx
    lea     ax,[bp-44]
    mov     si,ax

    mov     cx,44
    rep movsb

    pop     cx
    pop     bx
    pop     si
    pop     ds

; if it comes here it is successful
.success:
    xor     ax,ax

; return to the caller
.return:
    pop     bx
    mov     sp,bp
    pop     bp
    ret

; an error occured
.error:
    mov     ax,0FFFFh
    jmp     .return
; TEST: N/A


;===============================================
; Get the id of the FAT entry with the matching
; name.
;   In:
;     ds:si - File name
;   Out:
;     ax - Zero if failed; otherwise, the id.
;===============================================
fat_getEntryId:
    push    bp
    mov     bp,sp
    sub     sp,34
    pusha

    mov     word [bp-34],0

; search for the entry with the matching name
.search:
    push    es
    push    di
    push    bx

    mov     bx,ss
    mov     es,bx
    lea     di,[bp-32]

    mov     ax,word [bp-34]
    call    fat_rootGetEntry

    pop     bx
    pop     di
    pop     es

    test    ax,ax
    jne     .error

; compare the name of the entry on the stack to
; the one given by the user
.compare:
    push    es
    push    di
    push    bx
    push    cx

    mov     bx,ss
    mov     es,bx
    lea     di,[bp-32]
    
    mov     cx,11       ; name + extension

    ; [ds:si] [es:di] cx
;    xor     ax,ax
;    repe cmpsb
;    setne   al          ; set if not equal
    ; ^^ FIXME: not properly set...
    call    strncmp

    pop     cx
    pop     bx
    pop     di
    pop     es

    test    al,al       ; 0 = equal
    je      .return

    add     word [bp-34],1
    jmp     .search

; return to the caller
.return:
    popa
    mov     ax,word [bp-34]
    mov     sp,bp
    pop     bp
    ret

; an error occured
.error:
    mov     word [bp-34],0FFFFh
    jmp     .return
; TEST: OK


;===============================================
; Resizes a file to the given size.
;   In:
;     ds:si - File name
;     cx    - File size
;   Out:
;     ax - Zero if successful.
;===============================================
fat_fileResize:
    push    bp
    mov     bp,sp
    sub     sp,46

.init:
    mov     word [bp-46],cx

; searches for the entry with a matching name
.findExisting:
    mSearchEntry 44
    test    ax,ax
    jne     .error

; determine the number of clusters to use
.calcClusters:
    xor     dx,dx
    mov     ax,word [bp-46]                     ; size needed
    mov     cx,word [gs:iFAT_BytesPerSector]    ; ax = number of clusters needed
    div     cx                                  ; dx = modulo

    ; ensure the size fits
    test    dx,dx       ; if dx is not zero we
    je      .resize     ; will need another
    add     ax,1        ; cluster for the remainder

; actually resize
.resize:
    mov     cx,ax               ; number of clusters needed
    mov     ax,word [bp-18]     ; root/logical sector
    call    fat_relocClusters2
    test    ax,ax
    jne     .error

; update the file's entry
.update:
    mov     ax,word [bp-46]
    mov     word [bp-16],ax
    ; TODO: get index and update on the drive

; return to the caller
.return:
    mov     sp,bp
    pop     bp
    ret

; an error occured
.error:
    mov     ax,0FFFFh
    jmp     .return


;===============================================
; Reworked file writing function.
;===============================================
;   In:
;     bp+4  - File name (stack)
;     bp+6  - File name (offset)
;     bp+8  - File data (stack)
;     bp+10 - File data (offset)
;     bp+12 - File size
;===============================================
;   Old:
;     ds:si - File name
;     es:di - File data
;     cx    - File size
;===============================================
;   Out:
;     ax - Zero if successful.
;===============================================
fat_writeFile2:
    push    bp
    mov     bp,sp
    sub     sp,2

; TODO:

; return to the caller
.return:
    mov     sp,bp
    pop     bp
    ret
