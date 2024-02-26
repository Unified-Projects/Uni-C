@echo off
set input=./build/bin/Compiler.exe
set input2=./test/SimpleExit.uc
"%input%" -i "%input2%" -o test

nasm -f win64 test.asm
@REM ld test.obj -o test.exe --large-address-aware
link /MACHINE:X64 /SUBSYSTEM:CONSOLE /OUT:test.exe /NODEFAULTLIB /ENTRY:_start /LARGEADDRESSAWARE:NO "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64\kernel32.lib" test.obj
@REM objdump test.exe -D
@REM gcc -o test.exe test.obj -m64 -nostdlib -fPIE
test.exe
echo Return value is: %errorlevel%