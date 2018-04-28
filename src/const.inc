;===============================================
; SYSTEM WIDE CONSTANTS
;===============================================
%define MAX_PATH    256


; ax values for int 70h
%define INT_RESERVED        0
%define INT_LOAD_FILE       1
%define INT_EXEC_PROGRAM    2

%define INT_KEYPRESS        10h
%define INT_PRINT_STRING    11h
%define INT_PRINT_HEX       12h
%define INT_CLEAR_SCREEN    13h


; Key presses for get char
%define KEY_BACKSPACE   08h
%define KEY_TAB         09h
%define KEY_ENTER       0Dh
%define KEY_ESCAPE      1Bh

%define KEY_SPACE       20h
%define KEY_PERIOD      2Eh

%define KEY_0           30h
%define KEY_1           31h
%define KEY_2           32h
%define KEY_3           33h
%define KEY_4           34h
%define KEY_5           35h
%define KEY_6           36h
%define KEY_7           37h
%define KEY_8           38h
%define KEY_9           39h

%define KEY_A           41h
%define KEY_B           42h
%define KEY_C           43h
%define KEY_D           44h
%define KEY_E           45h
%define KEY_F           46h
%define KEY_G           47h
%define KEY_H           48h
%define KEY_I           49h
%define KEY_J           4Ah
%define KEY_K           4Bh
%define KEY_L           4Ch
%define KEY_M           4Dh
%define KEY_N           4Eh
%define KEY_O           4Fh
%define KEY_P           50h
%define KEY_Q           51h
%define KEY_R           52h
%define KEY_S           53h
%define KEY_T           54h
%define KEY_U           55h
%define KEY_V           56h
%define KEY_W           57h
%define KEY_X           58h
%define KEY_Y           59h
%define KEY_Z           5Ah

