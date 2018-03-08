@echo off
del *.exe
del *.obj
nasm -o example.obj -f win32 example.asm
cl -c -W4 -MD -Od -EHsc -DNDEBUG main.cpp
link /out:test.exe main.obj example.obj
