
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

.search_start:
    push    ax
    push    cx
    push    dx
    xchg    cx,ax           ; diff
    mov     ax,word [bp-4]  ; rootCluster

; loop till no more clusters are linked
.loop:
    test    ax,ax
    je      .search_end     ; 0 = unused

    cmp     ax,0FF8h
    jae     .search_end     ; >= 0FF8h last-of-file

    cmp     ax,0FF0h
    jae     .error          ; reserved/bad cluster

    mov     dx,ax           ; curCluster
    call    fat_getClusterValue
    jmp     .loop

.search_end:
    mov     word [bp-8],dx  ; last cluster of file
    pop     dx
    pop     cx          ; numClusters
    pop     ax          ; diff

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
