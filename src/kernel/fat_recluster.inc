
;===============================================
; Determines the current number of used clusters.
;   In:
;     ax - The starting cluster.
;   Out:
;     ax - Zero if successful.
;===============================================
fat_calcClusters:
    push    cx
    xor     cx,cx

; loop till no more clusters are linked
.loop:
    test    ax,ax       ; 0 = unused
    je      .return     ; >= 0FF0h reserved
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
    sub     bp,4

; store the initial values
.init:
    mov     word [bp-2],ax
    mov

; first set the cluster value of the new last of
; file to be the actual last-of-file.
.last:
    call    fat_getClusterValue     ; get the id of the next cluster
    mov     word [bp-4],ax          ; store the next cluster id

    mov     ax,word [bp-2]          ; current cluster id
    mov     cx,0FFFh                ; last-of-file
    call    fat_setClusterValue     ; change to last-of-file

; keep looping till no more cluster can be freed.
.loop:
    mov     ax,word [bp-4]
    test    ax,ax                   ; unused/free
    je      .return
    cmp     ax,0FF8h                ; error state(s)
    jae     .return
    cmp     ax,0FF0h                ; last-of-file
    jae     .last
    mov     word [bp-2],ax

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
    mov     cx,0
    call    fat_setClusterValue
    ;test    ax,ax
    ;just return the value
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
    sub     sp,6

    ; Store a copy of the value
    mov     word [bp-2],ax  ; reqClusters - The requested number of clusters.
    mov     word [bp-4],cx  ; rootCluster - The starting cluster.
    mov     word [bp-6],0   ; numClusters - The number of used clusters.

    ; The starting cluster will be our root
    mov     ax,cx

; main loop
.loop:
    mov     cx,word [bp-6]
    add     cx,1
    mov     word [bp-6],cx
    cmp     cx,word [bp-2]
    jne     .next

; last-of-file
.last:
    mov     ax,0FFFh
    call    fat_setClusterValue
    jmp     .return

; there is another cluster behind this cluster
.next:
    
    


; return to the caller
.return:
    mov     sp,bp
    pop     bp
    ret