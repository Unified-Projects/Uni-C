@echo off
set input=./build/bin/Compiler.exe
set input2=./test/SimpleExit.uc
"%input%" -i "%input2%" -o test2

nasm -f win64 test2.asm
@REM link test.obj /DEBUG /MACHINE:X64 /SUBSYSTEM:CONSOLE /OUT:test.exe /NODEFAULTLIB /ENTRY:_start /LARGEADDRESSAWARE:NO "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64\kernel32.lib" "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64\user32.lib"
@REM ld test.obj -o test.exe --large-address-aware
@REM objdump test.exe -D
@REM gcc -o test.exe test.obj -m64 -nostdlib -fPIE
gcc -o test.exe test2.obj -nostdlib -lkernel32
test.exe
echo Return value is: %errorlevel%