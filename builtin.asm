extern ExitProcess                          ;windows API function to exit process
extern WriteConsoleA                        ;windows API function to write to the console window (ANSI version)
extern ReadConsoleA                         ;windows API function to read from the console window (ANSI version)
extern GetStdHandle                         ;windows API to get the for the console handle for input/output

default rel                                 ;default to using RIP-relative addressing

section .data:
    STD_INPUT_HANDLE equ -10
    STD_OUTPUT_HANDLE equ -11
    STD_ERROR_HANDLE equ -12

    ; Convertions
    digits db "0123456789"
    NULL db 0

    ; TEST DATA
    testString db "Hello, World!", 0
    testStringLen equ $ - testString

section .bss:
    ; Reserved data
    Print_Written: resb 4
    Print_Chars: resb 4

section .text:
_PrintS: ; Print String
    mov rcx, STD_OUTPUT_HANDLE
    call GetStdHandle

    mov rcx, rax
    mov r9, Print_Written

    mov rax, qword 0
    mov qword [rsp + 0x20], rax

    call WriteConsoleA
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