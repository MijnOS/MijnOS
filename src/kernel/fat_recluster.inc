
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
;===============================================
fat_allocClusters:
    push    bp
    mov     bp,sp
    sub     sp,2

; keep looping till no more clusters have to be
; allocted.
.loop:
    mov     word [bp-2],ax                      ; store the current cluster

    call    fat_findEmptyCluster                ; find the next empty cluster
    cmp     ax,0FF0h
    jae     .return                             ; ensure error free

    push    cx
    mov     cx,ax                               ; the new value is the new cluster id
    mov     ax,word [bp-2]                      ; current cluster
    call    fat_setClusterValue                 ; set the cluster behind the current cluster to be the new cluster
    pop     cx

    loop    .loop

; set the last cluster to end-of-file
.last:
    call    fat_getClusterValue
    mov     cx,0FFFh
    call    fat_setClusterValue

; return to the caller
.return:
    mov     sp,bp
    pop     bp
    ret


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

; first set the cluster value of the new last of
; file to be the actual last-of-file.
.last:
    call    fat_getClusterValue
    push    ax
    mov     ax,word [bp-2]
    mov     cx,0FFFh
    call    fat_setClusterValue
    pop     ax

; keep looping till no more cluster can be freed.
.loop:
    mov     word [bp-2],ax
    call    fat_getClusterValue
    mov     word [bp-4],ax

    mov     ax,word [bp-2]                      ; clear current cluster
    mov     cx,0
    call    fat_setClusterValue

    mov     ax,word [bp-4]                      ; check next cluster
    test    ax,ax                               ; 0 = unused
    je      .return
    cmp     ax,0FF0h                            ; valid
    jb      .loop
    cmp     ax,0FF8h                            ; end-of-file
    jae     .loop
    ; x >= 0FF0h && x <= 0FF7h                  ; invalid

; return to the caller
.return:
    mov     sp,bp
    pop     bp
    ret









;===============================================
; Reallocates clusters. This includes growning
; as well as shrinking in number of clusters.
;   In:
;     ax - The number of necessary clusters.
;     cx - The starting cluster.
;   Out:
;     ax - Zero if successful.
;===============================================
fat_relocClusters:
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
