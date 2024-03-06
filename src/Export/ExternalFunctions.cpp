#include <Export/InternalFunctions.h>

std::map<std::string, CustomFunctionDeclaration> ExternalFunctions = {
    {
        "PrintString",
        {
            "_PrintS",
            {"WriteConsoleA", "GetStdHandle"},
            "_PrintS_Print_Written: resb 4\n_PrintS_Print_Chars: resb 4\n",
            "",
            "global _PrintS\n_PrintS: ; Print String\n    mov rcx, STD_OUTPUT_HANDLE\n    call GetStdHandle\n    mov rcx, rax\n    mov r9, _PrintS_Print_Written\n    mov rdx, [rdi + 8]\n    mov r8, [rdi]\n    call WriteConsoleA\n    ret\n"
        }
    }
};