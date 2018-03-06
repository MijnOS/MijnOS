[ORG 07C00h]                        ; Bootloaders are loaded to [0000:7C00]
[BITS 16]                           ; Bootloaders are 16-bits
[MAP ALL stack_test.map]            ; We want the NASM output for debugging
    jmp     near start


; Program entry-point
start:
    mov     ax,07C0h                ; Set the data and extra segments to the
    mov     ds,ax                   ; same segment as to where our code is
    mov     es,ax                   ; loaded too. This is a best practice
                                    ; approach.

                                    ; Set up the stack behind out bootloader.
    add     ax,32                   ; This points to 7C00h + 512 (32h) which is 
    mov     ss,ax                   ; directly after the 0AA55h marker.
    mov     sp,512                  ; The stack will be 512-bytes.
                                    ; The means the next available byte behind
                                    ; the stack is at 7C00h + 1024 (64h)

stack_test:
    ; Push test values onto the stack
    push    042h    ; 'B'
    push    043h    ; 'C'
    push    044h    ; 'D'
    push    045h    ; 'E'

    ; Set the stack
    mov     ax,07C0h
    mov     fs,ax

    ; Print the test string
    mov     ah,0Eh
    push    bx
    add     bx,1024                 ; Should be the offset to the top of the stack!
    mov     al,041h
    int     10h                     ; Should print 'A'
    mov     al,byte [fs:bx-2]
    int     10h                     ; Should print 'B'
    mov     al,byte [fs:bx-4]
    int     10h                     ; Should print 'C'
    mov     al,byte [fs:bx-6]
    int     10h                     ; Should print 'D'
    mov     al,byte [fs:bx-8]
    int     10h                     ; Should print 'E'
    mov     al,046h
    int     10h                     ; Should print 'F'
    pop     bx

    ; Hold the program here
    jmp     $


; Boot signature
times 510-($-$$) db 0
dw 0AA55h
