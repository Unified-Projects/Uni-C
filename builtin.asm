extern ExitProcess                          ;windows API function to exit process
extern WriteConsoleA                        ;windows API function to write to the console window (ANSI version)
extern ReadConsoleA                         ;windows API function to read from the console window (ANSI version)
extern GetStdHandle                         ;windows API to get the for the console handle for input/output

default rel                                 ;default to using RIP-relative addressing

section .data:

GenericConstants:
    STD_INPUT_HANDLE equ -10
    STD_OUTPUT_HANDLE equ -11
    STD_ERROR_HANDLE equ -12

CustomConstants:

    ; TEST DATA
    testString db "Hello, World!", 0
    testStringLen equ $ - testString

section .bss:
    ; Reserved data
    _PrintS_Print_Written: resb 4
    _PrintS_Print_Chars: resb 4

section .text:
global _PrintS
_PrintS: ; Print String
    ; sub rsp, 40
    mov rcx, STD_OUTPUT_HANDLE
    call GetStdHandle

    mov rcx, rax
    mov r9, _PrintS_Print_Written

    ; mov rax, qword 0
    ; mov qword [rsp + 0x20], rax

    mov rdx, [rdi + 8]
    mov r8, [rdi]

    call WriteConsoleA
    ; add rsp, 40
    ret

global _start
_start:
    ; Tester
    mov rdx, testString
    mov r8, testStringLen
    call _PrintS

    ; Exit
    call ExitProcess
    ret