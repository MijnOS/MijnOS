@echo off

:: Variables
SET LOADER=bootloader.bin
SET DEVICE=bootdevice.flp

:: Clean
echo Cleaning...
del *.flp
del *.bin
del *.exe

:: BUILD
echo Building...
cl -W4 -O2 writefloppy.cpp
nasm -f bin interrupt_vector_table.asm -o %LOADER%
writefloppy.exe %LOADER% %DEVICE%

:: Extra cleaning
del *.obj
