@echo off

:: Variables
SET LOADER=bootloader.bin
SET DEVICE=bootdevice.flp

:: BUILD
echo Building...
if not exist "..\bin" mkdir "..\bin"
cl -W4 -O2 -EHsc tools\writefloppy.cpp /link -out:"..\bin\writefloppy.exe"
nasm -f bin interrupt_vector_table.asm -o %LOADER%
..\bin\writefloppy.exe "..\bin\%DEVICE%" %LOADER%

:: Extra cleaning
echo Cleaning...
del *.bin
del *.obj
