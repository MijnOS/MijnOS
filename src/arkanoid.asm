[BITS 16]
; [ORG 0x15E00]
jmp main

%include "src\const.inc"

;===============================================
; Entry point
;===============================================
main:

    ; Switch to graphics mode
    mov     ax,INT_GPU_GRAPHICS
    int     70h

    ; Fill the screen with a white color with
    ; the exception of the edges
    mov     dx,198      ; y
.loop_0:
    mov     cx,318      ; x
.loop_1:
    mov     ax,INT_DRAW_PIXEL
    mov     bx,0Fh      ; Full white
    int     70h

    sub     cx,1
    jne     .loop_1     ; x_loop

    sub     dx,1
    jne     .loop_0     ; y_loop

    ; TEMP - Keypress
    mov     ax,INT_KEYPRESS
    int     70h

    retf
