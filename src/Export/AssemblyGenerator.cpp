#include <Export/AssemblyGenerator.h>
#include <CompilerInformation.h>

using namespace Exporting;
using namespace Exporting::Helpers;
using namespace Parsing;
using namespace Parsing::SemanticVariables;

#include <iostream>

/*
Optimisation plans

Build a get free regsiter function
and a get (variable) function that will return a register if it is in a register, remove it from the stack and put it in a register
It will move around registers and move them into stacks if they are not needed
*/

extern std::map<std::string, SemanticTypeDefinition> SemanticTypeMap;

/*
    // TypeName : DefinedScope
    std::pair<std::string, SemanticTypeDefinition>("int", {"int", 0, 4, {}, nullptr, 0, 0, 0}),
    std::pair<std::string, SemanticTypeDefinition>("uint", {"uint", 0, 4, {}, nullptr, 0, 0, 1}),
    std::pair<std::string, SemanticTypeDefinition>("short", {"short", 0, 2, {}, nullptr, 0, 0, 2}),
    std::pair<std::string, SemanticTypeDefinition>("ushort", {"ushort", 0, 2, {}, nullptr, 0, 0, 3}),
    std::pair<std::string, SemanticTypeDefinition>("long", {"long", 0, 8, {}, nullptr, 0, 0, 4}),
    std::pair<std::string, SemanticTypeDefinition>("ulong", {"ulong", 0, 8, {}, nullptr, 0, 0, 5}),
    std::pair<std::string, SemanticTypeDefinition>("float", {"float", 0, 4, {}, nullptr, 0, 0, 6}),
    std::pair<std::string, SemanticTypeDefinition>("double", {"double", 0, 8, {}, nullptr, 0, 0, 7}),
    std::pair<std::string, SemanticTypeDefinition>("char", {"char", 0, 1, {}, nullptr, 0, 0, 8}),
    std::pair<std::string, SemanticTypeDefinition>("uchar", {"uchar", 0, 1, {}, nullptr, 0, 0, 9}),
    std::pair<std::string, SemanticTypeDefinition>("bool", {"bool", 0, 1, {}, nullptr, 0, 0, 10}),
    std::pair<std::string, SemanticTypeDefinition>("void", {"void", 0, 1, {}, nullptr, 0, 0, 11}),
    std::pair<std::string, SemanticTypeDefinition>("string", {"string", 0, 1, {}, nullptr, 0, 0, 12}),
    std::pair<std::string, SemanticTypeDefinition>("*", {"*", 0, 8, {}, nullptr, 0, 0, 13}), // TODO Pointers
*/

std::map<int, std::vector<int>> CompatibleCombinations = {
    {0, {0, 1, 2, 3, 4, 5, 8, 9}},
    {1, {0, 1, 2, 3, 4, 5, 8, 9}},
    {2, {0, 1, 2, 3, 4, 5, 8, 9}},
    {3, {0, 1, 2, 3, 4, 5, 8, 9}},
    {4, {0, 1, 2, 3, 4, 5, 8, 9}},
    {5, {0, 1, 2, 3, 4, 5, 8, 9}},
    {6, {6, 7}},
    {7, {6, 7}},
    {8, {0, 1, 2, 3, 4, 5, 8, 9}},
    {9, {0, 1, 2, 3, 4, 5, 8, 9}},
    {10, {10}},
    {11, {11}},
    {12, {12, 8, 9}},
    {13, {13}},
    // TODO: Add Custom Compatible Combinations Using Operators
};

AssemblyGenerator::AssemblyGenerator(){
    stackTrace = new StackTrace();
    scopeTree = new ScopeTree();
    registerTable = new RegisterTable();
}
AssemblyGenerator::~AssemblyGenerator(){
    delete stackTrace;
    delete scopeTree;
    delete registerTable;
}

std::string AssemblyGenerator::ConvertFunction(Function* Function, int IndentCount){
    // Function Assembly
    std::string functionAssembly = std::string(IndentCount*4, ' ') + Function->Identifier + ":\n";

    for(auto& Lines : Function->Block){
        functionAssembly += ConvertGeneric(Lines, IndentCount+1);
    }

    return functionAssembly;
}

std::string AssemblyGenerator::ConvertGeneric(SemanticVariable* Block, int IndentCount){
    std::string assembly = "";
    switch (Block->Type())
    {
    case SemanticTypeFunction:
        {
            assembly += ConvertFunction((Function*)Block, IndentCount+1);
        }
        break;

    case SemanticTypeStatement:
        {
            switch (((SemStatement*)Block)->TypeID)
            {
            case SemanticStatementReturn:
                {
                    if(CompilerInformation::DebugAll()){
                        assembly += ";; Return Operation\n";
                    }
                    for(auto& Params : ((SemStatement*)Block)->Parameters){
                        if(Params->Type() == SemanticTypeOperation){
                            assembly += ConvertGeneric(Params, IndentCount+1);
                        }
                        else if(Params->Type() == SemanticTypeLiteral){
                            // Check if it is in data (Could be a string not a number) (Use ID to find it)
                            if(CompilerInformation::DebugAll()){
                                assembly += ";; Return Literal\n";
                            }

                            assembly += std::string(IndentCount*4, ' ') + "mov rax, " + ((SemLiteral*)Params)->Value + "\n";
                        }
                        else if(Params->Type() == SemanticTypeVariableRef){
                            // TODO: See if Variable is loaded to registers / Altered from initial value
                            // Check if it is in stack

                            // Check if it is in Data
                            // Variable Name
                            std::string VarName = ((VariableRef*)Params)->Identifier;

                            // Find in Scope Trace
                            Variable* var = (Variable*)scopeTree->FindVariable(VarName, Params);

                            if(var != nullptr){
                                if(var->InitValue != ""){
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Return Variable\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "mov rax, [" + GetSymbol(var) + "]\n";
                                }
                            }
                            else{
                                std::cerr << "Variable Not Found: " << VarName << std::endl;
                            }
                            
                            // Otherwise scope error
                        }
                    }
                    assembly += std::string(IndentCount*4, ' ') + "ret\n";
                }
                break;
            
            case SemanticStatementIf:
                {
                    if(CompilerInformation::DebugAll()){
                        assembly += ";; If Statement\n";
                    }

                    // Load Condition
                    if(((SemStatement*)Block)->Parameters.size() > 0){
                        for(auto& Params : ((SemStatement*)Block)->Parameters){
                            if(Params->Type() == SemanticTypeOperation){
                                assembly += ConvertGeneric(Params, IndentCount);
                            }
                        }
                    }
                    else{
                        std::cerr << "If Statement Without Condition" << std::endl;
                        return assembly;
                    }

                    // Now Compare Stack Value to 1
                    assembly += std::string(IndentCount*4, ' ') + registerTable->PullFromStack(stackTrace, "rax");
                    assembly += std::string(IndentCount*4, ' ') + "cmp rax, 1\n";
                    
                    if(((SemStatement*)Block)->Alternatives.size() >= 0){
                        // We need to jump to the If start
                        assembly += std::string(IndentCount*4, ' ') + "je IfStart_" + GetSymbol(Block) + "\n";
                    }

                    // Others
                    for(auto& Alternate : ((SemStatement*)Block)->Alternatives){
                        if(Alternate->Type() == SemanticTypeStatement){
                            SemStatement* SemState = (SemStatement*)Alternate;

                            if(SemState->TypeID == SemanticStatementElseIf){
                                if(CompilerInformation::DebugAll()){
                                    assembly += ";; Else If Statement\n";
                                }

                                // Load Condition
                                if(SemState->Parameters.size() > 0){
                                    for(auto& Params : SemState->Parameters){
                                        if(Params->Type() == SemanticTypeOperation){
                                            assembly += ConvertGeneric(Params, IndentCount);
                                        }
                                    }
                                }
                                else{
                                    std::cerr << "Else If Statement Without Condition" << std::endl;
                                    return assembly;
                                }

                                // Now Compare Stack Value to 1
                                assembly += std::string(IndentCount*4, ' ') + registerTable->PullFromStack(stackTrace, "rax");
                                assembly += std::string(IndentCount*4, ' ') + "cmp rax, 1\n";

                                assembly += std::string(IndentCount*4, ' ') + "je ElseIfStart_" + GetSymbol(Alternate) + "\n";
                            }
                            else if(SemState->TypeID == SemanticStatementElse){
                                if(CompilerInformation::DebugAll()){
                                    assembly += ";; Else Statement\n";
                                }

                                // Load Block
                                if(SemState->Block.size() > 0){
                                    for(auto& Lines : SemState->Block){
                                        assembly += ConvertGeneric(Lines, IndentCount + 1);
                                    }
                                }
                                else{
                                    std::cerr << "Else Statement Without Block" << std::endl;
                                    return assembly;
                                }

                                // End of If
                                assembly += std::string(IndentCount*4, ' ') + "jmp IfEnd_" + GetSymbol(Block) + "\n";
                            }
                        }
                    }

                    // If not true, jump to end
                    if(((SemStatement*)Block)->Alternatives.size() == 0){
                        assembly += std::string(IndentCount*4, ' ') + "jne IfEnd_" + GetSymbol(Block) + "\n";
                    }

                    assembly += std::string(IndentCount*4, ' ') + "IfStart_" + GetSymbol(Block) + ":\n";

                    // Load Block
                    if(((SemStatement*)Block)->Block.size() > 0){
                        for(auto& Lines : ((SemStatement*)Block)->Block){
                            assembly += ConvertGeneric(Lines, IndentCount+1);
                        }
                    }
                    else{
                        std::cerr << "If Statement Without Block" << std::endl;
                        return assembly;
                    }

                    // Others
                    for(auto& Alternate : ((SemStatement*)Block)->Alternatives){
                        if(((SemStatement*)Alternate)->TypeID == SemanticStatementElse){
                            continue; // Dont load else twice
                        }
                        assembly += std::string(IndentCount*4, ' ') + "jmp IfEnd_" + GetSymbol(Block) + "\n";
                        assembly += std::string(IndentCount*4, ' ') + "ElseIfStart_" + GetSymbol(Alternate) + ":\n";
                        for(auto& Lines : ((SemStatement*)Alternate)->Block){
                            assembly += ConvertGeneric(Lines, IndentCount+1);
                        }
                    }

                    // End of If
                    assembly += std::string(IndentCount*4, ' ') + "IfEnd_" + GetSymbol(Block) + ":\n";
                }
                break;
            
            default:
                break;
            }
        }
        break;
    
    case SemanticTypeOperation:
        {
            int EntitiesInStack = stackTrace->Size();

            // Use Stack For Operations
            for(auto& Params : ((Operation*)Block)->Parameters){
                if(Params->Type() == SemanticTypeOperation){
                    Operation* op = (Operation*)Params;

                    if(stackTrace->Size() - EntitiesInStack < 2){
                        // TODO allow operations that support 1 argument (Increment, Decrement, etc.)
                        if(CompilerInformation::DebugAll()){
                            assembly += ";; Operation Stack Unload\n";
                        }
                        assembly += std::string(IndentCount*4, ' ') + registerTable->PullFromStack(stackTrace, "r9");

                        switch (op->TypeID)
                        {
                        default:
                            std::cerr << "Not Enough Entities In Stack" << std::endl;
                            return assembly;
                            break;
                        }
                    }

                    // Get Values
                    if(CompilerInformation::DebugAll()){
                        assembly += ";; Operation Stack Unload\n";
                    }
                    assembly += std::string(IndentCount*4, ' ') + registerTable->PullFromStack(stackTrace, "r8");
                    assembly += std::string(IndentCount*4, ' ') + registerTable->PullFromStack(stackTrace, "r9");

                    // Compare Types
                    bool Calculated = false;
                    for(auto TypeID : CompatibleCombinations[registerTable->r9.TypeID]){
                        if(TypeID == registerTable->r8.TypeID){
                            // Now look into operations
                            switch (op->TypeID)
                            {
                            case SemanticOperationAddition:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Addition Operation\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + "add r9, r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r9");
                                }
                                break;

                            case SemanticOperationSubtraction:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Subtraction Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "sub r9, r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r9");
                                }
                                break;
                            
                            case SemanticOperationMultiplication:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Multiplication Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "imul r9, r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r9");
                                }
                                break;
                            
                            case SemanticOperationDivision:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Division Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + registerTable->MovReg("rax", "r9");
                                    assembly += std::string(IndentCount*4, ' ') + "cqo\n";
                                    registerTable->MovReg("rax", "rdx"); // Mimique cqo
                                    assembly += std::string(IndentCount*4, ' ') + "idiv r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "rax");
                                }
                                break;

                            case SemanticOperationModulus:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Modulus Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + registerTable->MovReg("rax", "r9");
                                    assembly += std::string(IndentCount*4, ' ') + "cqo\n";
                                    registerTable->MovReg("rax", "rdx"); // Mimique cqo
                                    assembly += std::string(IndentCount*4, ' ') + "idiv r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "rdx");
                                }
                                break;

                            case SemanticOperationAssignment:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Assignment Operation\n";
                                    }

                                    if(registerTable->r9.Var && !registerTable->r9.IsValue){
                                        assembly += std::string(IndentCount*4, ' ') + "mov r9, r8\n";
                                        assembly += std::string(IndentCount*4, ' ') + registerTable->r9.save();

                                        if(stackTrace->Size() - EntitiesInStack > 0){
                                            std::cerr << "Assignment in incomplete operations" << std::endl;
                                            return assembly;
                                        }
                                    }
                                    else{
                                        std::cerr << "Assignment to Non-Variable" << std::endl;
                                        return assembly;
                                    }
                                }
                                break;

                            case SemanticOperationEqual:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Equal Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "cmp r9, r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + "sete al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx r9, al\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r9");
                                }
                                break;

                            case SemanticOperationNotEqual:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Not Equal Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "cmp r9, r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + "setne al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx r9, al\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r9");
                                }
                                break;

                            case SemanticOperationLess:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Less Than Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "cmp r9, r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + "setl al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx r9, al\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r9");
                                }
                                break;

                            case SemanticOperationGreater:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Greater Than Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "cmp r9, r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + "setg al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx r9, al\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r9");
                                }
                                break;

                            case SemanticOperationLessEqual:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Less Than or Equal Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "cmp r9, r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + "setle al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx r9, al\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r9");
                                }
                                break;

                            case SemanticOperationGreaterEqual:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Greater Than or Equal Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "cmp r9, r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + "setge al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx r9, al\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r9");
                                }
                                break;

                            case SemanticOperationAnd:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; And Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "and r9, r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r9");
                                }
                                break;
                            
                            case SemanticOperationOr:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Or Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "or r9, r8\n";
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r9");
                                }
                                break;
                            
                            default:
                                break;
                            }

                            Calculated = true;
                        }
                    }

                    if(!Calculated){
                        std::cerr << "Incompatible Types: " << registerTable->r8.TypeID << " " << registerTable->r9.TypeID << std::endl;
                        return assembly;
                    }
                }
                else if(Params->Type() == SemanticTypeLiteral){
                    SemLiteral* lit = (SemLiteral*)Params;

                    if(CompilerInformation::DebugAll()){
                        assembly += ";; Load Literal\n";
                    }

                    // Load to a register
                    assembly += std::string(IndentCount*4, ' ') + registerTable->r8.set(lit);
                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r8");
                }
                else if(Params->Type() == SemanticTypeVariableRef){
                    VariableRef* var = (VariableRef*)Params;
                    
                    // Can we access variable reference
                    SemanticVariable* entry = scopeTree->FindVariable(var->Identifier, var);
                    if(entry != nullptr){
                        if(CompilerInformation::DebugAll()){
                            assembly += ";; Load Variable\n";
                        }

                        // Load register
                        assembly += std::string(IndentCount*4, ' ') + registerTable->r8.set((Variable*)entry, false);
                        assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r8");
                    }
                    else{
                        std::cerr << "Variable Not Found or Not In Scope: " << var->Identifier << std::endl;
                        return assembly;
                    }
                }
                else if(Params->Type() == SemanticTypeVariable){
                    Variable* var = (Variable*)Params;

                    if(CompilerInformation::DebugAll()){
                        assembly += ";; Load Variable\n";
                    }

                    // Load register
                    assembly += std::string(IndentCount*4, ' ') + registerTable->r8.set(var, false);
                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r8");
                }
            }

            // if(stackTrace->Size() - EntitiesInStack >= 1){
            //     std::cerr << "Incomplete Operations" << std::endl;
            //     return assembly;
            // }
        }
        break;

    default:
        break;
    }
    return assembly;
}

std::string AssemblyGenerator::Generate(const Parsing::SemanticVariables::SemanticVariable* RootScope){
    // Basic entry assembly that should be consistant
    std::string codeAssembly = "section .text\n";

    if(CompilerInformation::DebugAll()){
        codeAssembly += ";; External Symbols\n";
    }

    // Locate External Symbols
    codeAssembly += "extern ExitProcess\n";

    /*
extern ExitProcess; rcx DWORD ExitCode
extern CreateFileA; rcx LPCSTR lpFileName, rdx DWORD dwDesiredAccess
extern CreateFileW; rcx LPCWSTR lpFileName, rdx DWORD dwDesiredAccess
extern ReadFile; rcx HANDLE hFile, rdx LPVOID lpBuffer
extern WriteFile; rcx HANDLE hFile, rdx LPCVOID lpBuffer
extern CloseHandle; rcx HANDLE hObject
extern CreateThread; rcx LPSECURITY_ATTRIBUTES lpThreadAttributes, rdx SIZE_T dwStackSize
extern CreateProcessA; rcx LPCSTR lpApplicationName, rdx LPSTR lpCommandLine
extern CreateProcessW; rcx LPCWSTR lpApplicationName, rdx LPWSTR lpCommandLine
extern Sleep; rcx DWORD dwMilliseconds
extern GetTickCount
extern GetLastError
extern VirtualAlloc; rcx SIZE_T dwSize, rdx DWORD flAllocationType
extern VirtualFree; rcx LPVOID lpAddress, rdx SIZE_T dwSize
extern GetModuleHandleA; rcx LPCSTR lpModuleName
extern GetModuleHandleW; rcx LPCWSTR lpModuleName
extern GetProcAddress; rcx HMODULE hModule, rdx LPCSTR lpProcName
extern LoadLibraryA; rcx LPCSTR lpLibFileName
extern LoadLibraryW; rcx LPCWSTR lpLibFileName
extern FreeLibrary; rcx HMODULE hLibModule
extern WaitForSingleObject; rcx HANDLE hHandle, rdx DWORD dwMilliseconds
extern GetCommandLineA
extern GetCommandLineW
extern GetTickCount64
extern GetCurrentThreadId
extern GetProcessId; rcx HANDLE Process
extern GetModuleFileNameA; rcx HMODULE hModule, rdx LPSTR lpFilename
extern GetModuleFileNameW; rcx HMODULE hModule, rdx LPWSTR lpFilename
extern GetSystemTime; rcx LPSYSTEMTIME lpSystemTime
extern SetConsoleCursorPosition; rcx HANDLE hConsoleOutput, rdx COORD dwCursorPosition
extern GetConsoleScreenBufferInfo; rcx HANDLE hConsoleOutput, rdx PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo
extern SetConsoleTextAttribute; rcx HANDLE hConsoleOutput, rdx WORD wAttributes
extern GetConsoleMode; rcx HANDLE hConsoleHandle, rdx LPDWORD lpMode
extern SetConsoleMode; rcx HANDLE hConsoleHandle, rdx DWORD dwMode
extern GetCurrentProcess
extern SetProcessAffinityMask; rcx HANDLE hProcess, rdx DWORD_PTR dwProcessAffinityMask
extern GetThreadPriority; rcx HANDLE hThread
extern SetThreadPriority; rcx HANDLE hThread, rdx int nPriority
extern GetSystemInfo; rcx LPSYSTEM_INFO lpSystemInfo
extern QueryPerformanceCounter; rcx PLARGE_INTEGER lpPerformanceCount
extern GetEnvironmentVariableA; rcx LPCSTR lpName, rdx LPSTR lpBuffer
extern GetEnvironmentVariableW; rcx LPCWSTR lpName, rdx LPWSTR lpBuffer
extern SetEnvironmentVariableA; rcx LPCSTR lpName, rdx LPCSTR lpValue
extern SetEnvironmentVariableW; rcx LPCWSTR lpName, rdx LPCWSTR lpValue
extern GetFileSize; rcx HANDLE hFile, rdx LPDWORD lpFileSizeHigh
extern SetFilePointer; rcx HANDLE hFile, rdx LONG lDistanceToMove
extern GetTempPathA; rcx DWORD nBufferLength, rdx LPSTR lpBuffer
extern GetTempPathW; rcx DWORD nBufferLength, rdx LPWSTR lpBuffer
extern GetTempFileNameA; rcx LPCSTR lpPathName, rdx LPCSTR lpPrefixString
extern GetTempFileNameW; rcx LPCWSTR lpPathName, rdx LPCWSTR lpPrefixString
extern DeleteFileA; rcx LPCSTR lpFileName
extern DeleteFileW; rcx LPCWSTR lpFileName
extern CopyFileA; rcx LPCSTR lpExistingFileName, rdx LPCSTR lpNewFileName
extern CopyFileW; rcx LPCWSTR lpExistingFileName, rdx LPCWSTR lpNewFileName
extern MoveFileA; rcx LPCSTR lpExistingFileName, rdx LPCSTR lpNewFileName
extern MoveFileW; rcx LPCWSTR lpExistingFileName, rdx LPCWSTR lpNewFileName
    */
    
    if(CompilerInformation::DebugAll()){
        codeAssembly += ";; Standard Start Function (Windows only)\n";
    }
    codeAssembly += "\nglobal _start\n_start:\n    call main\n    mov rcx, rax\n    call ExitProcess\n\n\nFunctionSpaceStart:\n";
    // codeAssembly += "global _start\n_start:\n    call main\n    mov rdi, rax\n    mov rax, 60\n    syscall\n\nFunctionSpaceStart:\n";
    // std::string codeAssembly = "section .text\nglobal _start\n_start:\n    call main\n    mov ebx, eax\n    mov eax, 1\n    int 0x80\n\nFunctionSpaceStart:\n";
    std::string dataAssembly = "section .data\nDataBlockStart:\n";

    // Generate Scope Tree
    scopeTree->Generate((SemanticVariable*)RootScope);

    // Load Data
    if(CompilerInformation::DebugAll()){
        dataAssembly += ";; Defined Symbols\n";
    }
    dataAssembly += RecurseTreeForData((SemanticVariable*)RootScope);

    // Load Code
    for(auto& MainScopes : ((Scope*)RootScope)->Block){
        // Each scope here is a head scope and should be handled separately
        for(auto& ScopeContent : ((Scope*)MainScopes)->Block){
            if(ScopeContent->Type() == SemanticTypeFunction){
                if(CompilerInformation::DebugAll()){
                    std::string Identifier = ((Function*)ScopeContent)->Identifier + "(";
                    for(auto& Params : ((Function*)ScopeContent)->Parameters){
                        Variable* Par = (Variable*)Params;
                        Identifier += Par->Identifier + ", ";
                    }
                    Identifier += ")";
                    codeAssembly += ";; Function: " + Identifier + " Start\n";
                }

                codeAssembly += ConvertFunction((Function*)ScopeContent, 0);
            }
        }
    }

    return ";; This was automatically generated using the Uni-C Compiler\nbits 64\n\n" + dataAssembly + "DataBlockEnd:\n\n" + codeAssembly + "FunctionSpaceEnd:\n";
}

std::string AssemblyGenerator::RecurseTreeForData(SemanticVariable* Block){
    std::string Data = "";

    switch (Block->Type())
    {
    case SemanticTypeVariable:
        {
            Data += StoreVariable((Variable*)Block);
        }
        break;
    case SemanticTypeLiteral:
        {
            Data += StoreLiteral((SemLiteral*)Block);
        }
        break;
    case SemanticTypeFunction:
        {
            for(auto& Lines : ((Function*)Block)->Block){
                Data += RecurseTreeForData(Lines);
            }
        }
        break;
    case SemanticTypeScope:
        {
            for(auto& Lines : ((Scope*)Block)->Block){
                Data += RecurseTreeForData(Lines);
            }
        }
        break;
    case SemanticTypeStatement:
        {
            for(auto& Lines : ((SemStatement*)Block)->Parameters){
                Data += RecurseTreeForData(Lines);
            }
            for(auto& Lines : ((SemStatement*)Block)->Block){
                Data += RecurseTreeForData(Lines);
            }
        }
        break;
    case SemanticTypeOperation:
        {
            for(auto& Lines : ((Operation*)Block)->Parameters){
                Data += RecurseTreeForData(Lines);
            }
        }
        break;
    case SemanticTypeType:
        {
            for(auto& Lines : ((SemTypeDef*)Block)->Attributes){
                Data += RecurseTreeForData(Lines);
            }
            for(auto& Lines : ((SemTypeDef*)Block)->Methods){
                Data += RecurseTreeForData(Lines);
            }
        }
        break;
    
    default:
        break;
    }

    return Data;
}

std::string AssemblyGenerator::StoreVariable(Variable* Var){
    std::string Data = "";

    // A Variable Only Needs Its Initial Data Stored (If Exists)
    if(Var->InitValue != ""){
        if(Var->TypeID == SemanticTypeMap["int"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dd " + Var->InitValue + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["uint"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dd " + Var->InitValue + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["char"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " db " + Var->InitValue + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["uchar"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " db " + Var->InitValue + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["short"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dw " + Var->InitValue + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["ushort"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dw " + Var->InitValue + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["long"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dq " + Var->InitValue + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["ulong"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dq " + Var->InitValue + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["string"].TypeID){
            // Value is a string
            Data += GetSymbol(Var) + " db " + Var->InitValue + ", 0\n";
        }
    }

    return Data;
}

std::string AssemblyGenerator::StoreLiteral(SemLiteral* Lit){
    std::string Data = "";

    // A literal Only Needs to be stored if it is a string
    if(Lit->TypeID == SemanticTypeMap["string"].TypeID){
        Data += GetSymbol(Lit) + " db \"" + Lit->Value + "\", 0\n";
    }

    return Data;
}
