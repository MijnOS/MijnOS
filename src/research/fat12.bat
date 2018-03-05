cls
nasm -f bin -o fat12.bin fat12.asm
..\..\bin\writefloppy.exe -o fat12.flp fat12.bin
