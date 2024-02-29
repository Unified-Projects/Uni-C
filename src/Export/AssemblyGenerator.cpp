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

    if(Function->Block.back()->Type() != SemanticTypeStatement && ((SemStatement*)Function->Block.back())->TypeID != SemanticStatementReturn){
        functionAssembly += registerTable->CorrectStack(stackTrace, IndentCount);
        functionAssembly += std::string(IndentCount*4, ' ') + "ret\n";
    }
    else{
        // Remove Save
        registerTable->CorrectStack(stackTrace, IndentCount);
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
                            assembly += ConvertGeneric(Params, IndentCount);
                        }
                    }

                    // Stack Correction
                    assembly += registerTable->CorrectStackNoSaveDec(stackTrace, IndentCount);

                    assembly += std::string(IndentCount*4, ' ') + "ret\n";
                }
                break;
            
            case SemanticStatementIf:
                {
                    // Stack save point
                    registerTable->NewSave(stackTrace);

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
                    auto CmpReg = registerTable->GetFreeReg(stackTrace, assembly, IndentCount);
                    assembly += std::string(IndentCount*4, ' ') + registerTable->PullFromStack(stackTrace, CmpReg->Register);
                    assembly += std::string(IndentCount*4, ' ') + "cmp " + CmpReg->get() + ", 1\n";
                    registerTable->ReleaseReg(CmpReg);
                    
                    if(((SemStatement*)Block)->Alternatives.size() > 0){
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
                                CmpReg = registerTable->GetFreeReg(stackTrace, assembly, IndentCount);
                                assembly += std::string(IndentCount*4, ' ') + registerTable->PullFromStack(stackTrace, CmpReg->Register);
                                assembly += std::string(IndentCount*4, ' ') + "cmp " + CmpReg->get() + ", 1\n";
                                registerTable->ReleaseReg(CmpReg);

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
                    
                    // Restore stack
                    assembly += registerTable->CorrectStack(stackTrace, IndentCount);
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

            std::vector<SemanticVariable*> Arguments = {};

            // Use Stack For Operations
            for(auto& Params : ((Operation*)Block)->Parameters){
                if(Params->Type() == SemanticTypeOperation){
                    if(((Operation*)Params)->TypeID == SemanticOperationFunctionCall){
                        if(CompilerInformation::DebugAll()){
                            assembly += ";; Function Call\n";
                        }

                        // Find function
                        FunctionRef* FuncRef = (FunctionRef*)((Operation*)Params)->Parameters[0];
                        auto Func = scopeTree->FindFunction(FuncRef->Identifier, FuncRef);

                        if(!Func || Func->Type() != SemanticTypeFunction){
                            std::cerr << "Function Not Found: " << FuncRef->Identifier << std::endl;
                            return assembly;
                        }

                        // Load paramenters
                        bool Skipped = false;
                        for(auto& Params : ((Operation*)Params)->Parameters){
                            if(!Skipped){
                                Skipped = true;
                                continue;
                            }
                            if(Params->Type() == SemanticTypeOperation){
                                assembly += ConvertGeneric(Params, IndentCount);
                            }
                        }

                        auto reg = registerTable->GetVariable(stackTrace, FuncRef, assembly, IndentCount);

                        // Call Function
                        assembly += std::string(IndentCount*4, ' ') + "call " + FuncRef->Identifier + "\n";
                        registerTable->ReleaseReg(&registerTable->rax);
                        assembly += std::string(IndentCount*4, ' ') + "mov " + reg->get() + ", eax\n";

                        Arguments.push_back(FuncRef);
                        continue;
                    }

                    Operation* op = (Operation*)Params;
                    // VariableRefs
                    auto A2 = Arguments.back();
                    Arguments.pop_back();
                    auto A1 = Arguments.back();

                    Exporting::Helpers::RegisterValue* Arg1 = nullptr;
                    Exporting::Helpers::RegisterValue* Arg2 = nullptr;

                    if(A2->Type() == SemanticTypeRegisterRef){
                        Arg2 = registerTable->GetVariable(stackTrace, A2, assembly, IndentCount);
                        Arg1 = registerTable->GetVariable(stackTrace, A1, assembly, IndentCount);
                    }
                    else{
                        Arg1 = registerTable->GetVariable(stackTrace, A1, assembly, IndentCount);
                        Arg2 = registerTable->GetVariable(stackTrace, A2, assembly, IndentCount);
                    }

                    // Compare Types
                    bool Calculated = false;
                    for(auto TypeID : CompatibleCombinations[Arg1->TypeID]){
                        if(TypeID == Arg2->TypeID){
                            // Now look into operations
                            switch (op->TypeID)
                            {
                            case SemanticOperationAddition:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Addition Operation\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + "add " + Arg1->get() + ", " + Arg2->get() + "\n";
                                }
                                break;

                            case SemanticOperationSubtraction:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Subtraction Operation\n";
                                    }

                                    assembly += std::string(IndentCount*4, ' ') + "sub " + Arg1->get() + ", " + Arg2->get() + "\n";
                                }
                                break;
                            
                            case SemanticOperationMultiplication:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Multiplication Operation\n";
                                    }

                                    // TODO NON INTS

                                    assembly += std::string(IndentCount*4, ' ') + "imul " + Arg1->get() + ", " + Arg2->get() + "\n";
                                }
                                break;
                            
                            case SemanticOperationDivision:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Division Operation\n";
                                    }

                                    if(Arg1->Register != "rax"){
                                        assembly += std::string(IndentCount*4, ' ') + registerTable->MovReg(stackTrace, "rax", Arg1->get());
                                        registerTable->ReleaseReg(Arg1);
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + "cqo\n";
                                    registerTable->MovReg(stackTrace, "rax", "rdx"); // Mimique cqo
                                    assembly += std::string(IndentCount*4, ' ') + "idiv " + Arg2->get() + "\n";
                                    registerTable->ReleaseReg(&registerTable->rdx);
                                }
                                break;

                            case SemanticOperationModulus: // Division?
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Modulus Operation\n";
                                    }

                                    if(Arg1->Register != "rax"){
                                        assembly += std::string(IndentCount*4, ' ') + registerTable->MovReg(stackTrace, "rax", Arg1->get());
                                        registerTable->ReleaseReg(Arg1);
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + "cqo\n";
                                    registerTable->MovReg(stackTrace, "rax", "rdx"); // Mimique cqo
                                    assembly += std::string(IndentCount*4, ' ') + "idiv " + Arg2->get() + "\n";
                                    registerTable->ReleaseReg(&registerTable->rdx);
                                    
                                }
                                break;

                            case SemanticOperationAssignment:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Assignment Operation\n";
                                    }

                                    Arguments.pop_back();

                                    if(Arg1->Var->Type() != SemanticTypeLiteral){
                                        assembly += std::string(IndentCount*4, ' ') + "mov " + Arg1->get() + ", " + Arg2->get() + "\n";

                                        std::string Save = Arg1->save();
                                        if(Save.size() > 0){
                                            assembly += std::string(IndentCount*4, ' ') + Save;
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

                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "push rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + "cmp " + Arg1->get() + ", " + Arg2->get() + "\n";
                                    assembly += std::string(IndentCount*4, ' ') + "sete al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx " + Arg1->get() + ", al\n";
                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "pop rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, Arg1->Register);
                                }
                                break;

                            case SemanticOperationNotEqual:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Not Equal Operation\n";
                                    }

                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "push rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + "cmp " + Arg1->get() + ", " + Arg2->get() + "\n";
                                    assembly += std::string(IndentCount*4, ' ') + "setne al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx " + Arg1->get() + ", al\n";
                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "pop rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, Arg1->Register);
                                }
                                break;

                            case SemanticOperationLess:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Less Than Operation\n";
                                    }

                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "push rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + "cmp " + Arg1->get() + ", " + Arg2->get() + "\n";
                                    assembly += std::string(IndentCount*4, ' ') + "setl al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx " + Arg1->get() + ", al\n";
                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "pop rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, Arg1->Register);
                                }
                                break;

                            case SemanticOperationGreater:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Greater Than Operation\n";
                                    }

                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "push rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + "cmp " + Arg1->get() + ", " + Arg2->get() + "\n";
                                    assembly += std::string(IndentCount*4, ' ') + "setg al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx " + Arg1->get() + ", al\n";
                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "pop rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, Arg1->Register);
                                }
                                break;

                            case SemanticOperationLessEqual:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Less Than or Equal Operation\n";
                                    }

                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "push rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + "cmp " + Arg1->get() + ", " + Arg2->get() + "\n";
                                    assembly += std::string(IndentCount*4, ' ') + "setle al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx " + Arg1->get() + ", al\n";
                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "pop rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, Arg1->Register);
                                }
                                break;

                            case SemanticOperationGreaterEqual:
                                {
                                    if(CompilerInformation::DebugAll()){
                                        assembly += ";; Greater Than or Equal Operation\n";
                                    }

                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "push rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + "cmp " + Arg1->get() + ", " + Arg2->get() + "\n";
                                    assembly += std::string(IndentCount*4, ' ') + "setge al\n";
                                    assembly += std::string(IndentCount*4, ' ') + "movzx " + Arg1->get() + ", al\n";
                                    if(Arg2->Register != "rax" && Arg1->Register != "rax" && registerTable->rax.Var != nullptr){
                                        assembly += std::string(IndentCount*4, ' ') + "pop rax\n";
                                    }
                                    assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, Arg1->Register);
                                }
                                break;
                            
                            default:
                                break;
                            }

                            Calculated = true;
                        }
                    }

                    if(!Calculated){
                        std::cerr << "Incompatible Types: " << Arg1->TypeID << " " << Arg2->TypeID << std::endl;
                        return assembly;
                    }
                }
                else if(Params->Type() == SemanticTypeLiteral){
                    SemLiteral* lit = (SemLiteral*)Params;

                    // Load to a register
                    // assembly += std::string(IndentCount*4, ' ') + registerTable->r8.set(lit);
                    // assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r8");
                    Arguments.push_back(lit);
                }
                else if(Params->Type() == SemanticTypeVariableRef){
                    VariableRef* var = (VariableRef*)Params;
                    
                    // Can we access variable reference
                    SemanticVariable* entry = scopeTree->FindVariable(var->Identifier, var);
                    if(entry != nullptr){

                        // Load register
                        // assembly += std::string(IndentCount*4, ' ') + registerTable->r8.set((Variable*)entry, false);
                        // assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r8");
                        Arguments.push_back((Variable*)entry);
                    }
                    else{
                        std::cerr << "Variable Not Found or Not In Scope: " << var->Identifier << std::endl;
                        return assembly;
                    }
                }
                else if(Params->Type() == SemanticTypeVariable){
                    Variable* var = (Variable*)Params;

                    // Load register
                    // assembly += std::string(IndentCount*4, ' ') + registerTable->r8.set(var, false);
                    // assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, "r8");
                    Arguments.push_back(var);
                }
                else if(Params->Type() == SemanticTypeRegisterRef){
                    RegisterRef* reg = (RegisterRef*)Params;

                    // Load register
                    // assembly += std::string(IndentCount*4, ' ') + registerTable->PushToStack(stackTrace, reg->Register);
                    Arguments.push_back(reg);
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

                // Create Stack store
                registerTable->NewSave(stackTrace);

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
            for(auto& Lines : ((Function*)Block)->Parameters){
                Data += RecurseTreeForData(Lines);
            }
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
    if(Var->InitValue.size() > 0){
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
    else{
        if(Var->TypeID == SemanticTypeMap["int"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dd " + "0" + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["uint"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dd " + "0" + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["char"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " db " + "0" + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["uchar"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " db " + "0" + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["short"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dw " + "0" + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["ushort"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dw " + "0" + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["long"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dq " + "0" + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["ulong"].TypeID){
            // Value is a integer
            Data += GetSymbol(Var) + " dq " + "0" + "\n";
        }
        else if(Var->TypeID == SemanticTypeMap["string"].TypeID){
            // Value is a string
            Data += GetSymbol(Var) + " db " + "0" + ", 0\n";
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
