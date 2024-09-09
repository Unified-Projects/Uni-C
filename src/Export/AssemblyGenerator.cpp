#include <Export/AssemblyGenerator.h>
#include <CompilerInformation.h>

#include <algorithm>
#include <stack>

#include <Export/InternalFunctions.h>

using namespace Exporting;
using namespace Exporting::Helpers;
using namespace Parsing;
using namespace Parsing::SemanticVariables;

#include <iostream>

// Registers
// R8-15 are Generic and usable
// RAX - Return value
// RSP - Stack reserve
// RBX-RDX RSI,RDI,RBP - Argument (WinAPI uses RCX, RDX, R8, R9)

// XMM0-XMM15 - FLOAT

extern std::map<std::string, SemanticTypeDefDeclaration*> StandardisedTypes;
extern std::map<SemanticTypeDefDeclaration*, SemanticObjectDefinition*> StandardObjectTypes;

extern std::string GenerateID();

std::vector<std::string> ArgumentRegisters {
    "rbx",
    "rcx",
    "rdx",
    "rsi",
    "rdi",
    "rbp",
};

std::string AssemblyGenerator::InterpretOperation(SemanticisedFile* File, SemanticFunctionDeclaration* Function, SemanticOperation* Operation, SemanticBlock* ParentBlock){
    std::string Assembly = "";

    // Two registers are needed
    std::string OperationID = GenerateID();
    std::string Temp1Symbol = GenerateID();
    std::string Temp2Symbol = GenerateID();
    RegisterDefinition* Register1 = this->Stack->GetTempRegister(Temp1Symbol, Assembly, OperationID);
    RegisterDefinition* Register2 = this->Stack->GetTempRegister(Temp2Symbol, Assembly, OperationID);

    std::stack<std::string> Types = {};

    // Interpret operations
    for(auto Oper : Operation->Operations){
        if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VALUE){
            if(Register1->SetVal(Oper) == ""){
                std::cerr << "ASM::" << __LINE__ << " (" << Oper->TypeDef << ") cannot be used in operation" << std::endl;
                return "";
            }
            Assembly += Register1->SetVal(Oper);
            Assembly += "    push " + Register1->Get(8) + "\n";
            Types.push(Oper->TypeDef);
        }
        else if (Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VARIABLE){
            auto Var = Stack->FindSymbol(Oper->Value, Oper->TypeDef, Assembly);
            Assembly += "    push " + Var->Get(8) + "\n";
            Types.push(Oper->TypeDef);
        }
        else if (Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VARIABLE_ARGUMENT){
            if (atoi(Oper->Value.data()) > ArgumentRegisters.size()){
                // TODO GET FROM STACK
                std::cerr << __LINE__ << " STUB" << std::endl;
            }
            Assembly += "    push " + ArgumentRegisters[std::atoi(Oper->Value.data())] + "\n";
            Types.push(Oper->TypeDef);
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_FUNCTION){
            for(auto p : Oper->Values){
                std::string Parameter = InterpretOperation(File, Function, p, ParentBlock);
                if(Parameter == ""){
                    return "";
                }
                Assembly += Parameter;
            }

            // Save stack for allignment
            //TODO Stack helper tool to do this and allow stack based argumetns
            Assembly += "    mov q [__" + File->ID + "__" + Function->Symbol + "__RSP_8], rsp\n    and q rsp, rsp, -64\n";

            Assembly += "    call " + Oper->Value + "\n";

            Assembly += "    mov q rsp, [__" + File->ID + "__" + Function->Symbol + "__RSP_8]\n";

            Assembly += "    push rax\n"; // Load return value

            Types.push(Oper->TypeDef);
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_ADD){
            Assembly += "    pop " + Register1->Get(8) + "\n";
            Assembly += "    pop " + Register2->Get(8) + "\n";

            // TODO Non-Standard operators!!!

            std::string Reg1Type = Types.top();
            Types.pop();
            std::string Reg2Type = Types.top();
            Types.pop();

            if(StandardisedTypes[Reg1Type]->DataSize > StandardisedTypes[Reg2Type]->DataSize){
                Assembly += "    add " + Register1->GetBitOp(StandardisedTypes[Reg1Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Types.push(Reg1Type);
            }
            else{
                Assembly += "    add " + Register1->GetBitOp(StandardisedTypes[Reg2Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg2Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Types.push(Reg2Type);
            }
        }
        else{
            std::cerr << __LINE__ << " Operation Not implemented" << std::endl;
            return "";
        }
    }

    // Result store
    if(Operation->ResultStore != ""){
        // There is a place to put it
        if(Operation->ResultStore.find_first_of("__") != std::string::npos){
            // Variable / TempVarStore
            std::cerr << __LINE__ << " Not implemented" << std::endl;
            return "";
        }
        else{
            // Register store
            Assembly += "    pop " + Operation->ResultStore + "\n";
        }
    }

    // Free all registers used
    this->Stack->ReleaseTempRegister(Temp1Symbol, Assembly);
    this->Stack->ReleaseTempRegister(Temp2Symbol, Assembly);

    return Assembly;
}

std::string AssemblyGenerator::GenerateBlock(SemanticisedFile* File, SemanticFunctionDeclaration* Function, SemanticBlock* Block){
    std::string Assembly = "";
    bool HasReturned = false;

    for (auto s : Block->Block){
        if(s->GetType() == 0){ // Statement
            SemanticStatment* Statement = (SemanticStatment*)s;
            if(Statement->StateType == SemanticStatment::StatementType::RETURN_STATEMENT){
                HasReturned = true;
                if(Statement->ParameterOperations.size() > 0){
                    std::string Operation = InterpretOperation(File, Function, (SemanticOperation*)(Statement->ParameterOperations[0]), Block);

                    if(Operation.size() <= 0){
                        std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Statement Opperation failed to generate" << std::endl;
                        return "";
                    }

                    Assembly += Operation;
                }
                
                Assembly += "    mov q rsp, [__" + File->ID + "__" + Function->Symbol + "__RSP]\n    ret\n";
            }
            else{
                std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Statement not implemented (" << Statement->StateType << ")" << std::endl;
                return "";
            }
        }
        else{
            std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Failed to interpret block element" << std::endl;
            return "";
        }
    }

    if(!HasReturned && Function->Block == Block /*Make sure we are in main block for function return condition*/){
        std::cerr << Function->Identifier << " << ASM::" << __LINE__ << " << " << "Function did not return" << std::endl;
        return "";
    }

    return Assembly;
}

std::string AssemblyGenerator::Generate(Parsing::SemanticAnalyser* Files){
    std::string File_Header = ";; This was automatically generated using the Uni-C Compiler for Uni-CPU\n";
    std::string File_BSS = ""; // Un-Initialised Data (Zeroed)
    std::string File_DATA = ""; // Initialised Data
    std::string File_Text = ""; // Code and Consts

    //
    // Load global Variables
    //

    //
    // Load Constants (Non-Global)
    //

    //
    // Allocate BSS for (Non-Global) Non-Standard Types
    //

    //
    // Load External Functions
    //

    //
    // Load Functions
    //
    std::string LoadedFunctions = "";
    {
        for(auto F : Files->GetFiles()){
            for(auto f : F->FunctionDefs){

                std::string FunctionString = "";
                
                // Load function
                FunctionString = GenerateBlock(F, f, f->Block);

                if(FunctionString == ""){
                    std::cerr << F->AssociatedFile << ":" << f->Identifier << " << ASM::" << __LINE__ << " << " << "Failed to build function" << std::endl;
                    // return ""; // TODO Enable
                }

                // Load header
                FunctionString = "; global __" + F->ID + "__" + f->Symbol + "\n__" + F->ID + "__" + f->Symbol + ":\n    mov q [__" + F->ID + "__" + f->Symbol + "__RSP], rsp\n" + FunctionString;
                File_BSS += "__" + F->ID + "__" + f->Symbol + "__RSP:\n    resb 8\n";
                File_BSS += "__" + F->ID + "__" + f->Symbol + "__RSP_8:\n    resb 8\n";

                LoadedFunctions += FunctionString;
            }
        }
    }
    File_Text += LoadedFunctions;

    //
    // Final Assembly stages
    //

    if(File_BSS.size() + File_DATA.size() + File_Text.size() <= 0){
        std::cerr << "ASM::" << __LINE__ << " << " << "No File Contents Generated" << std::endl;
        // return ""; // TODO ENABLE
    }

    //
    // Entry Function
    //

    std::string EntryText = "";
    // Sort out entry to the program
    {
        // Locate entry function
        std::string EntryFunction = "";
        std::string RetVal = "";
        { // Find entry function in files (First come first served) (Match function style)
            for(auto F : Files->GetFiles()){
                for(auto f : F->FunctionDefs){
                    if(f->Identifier == "main" && f->Namespace == "__GLOB__" && f->StartToken && f->Parameters.size() == 0 /*TODO Arg stuff*/){
                        if(!f->FunctionReturn.WillReturn){
                            RetVal = "\n    mov q rax, 0";
                        }
                        else if(f->FunctionReturn.TypeDef == "int"){
                            // Nothinghere just a preventative method
                        }
                        else continue;
                        // Suitable
                        EntryFunction = "__" + F->ID + "__" + f->Symbol;
                        break;
                    }
                }

                if(EntryFunction != ""){
                    break;
                }
            }
        }

        // Sort out stack allignment
        std::string EntryRSPSave = GenerateID() + "__RSP__STORE";
        File_BSS = "\n" + EntryRSPSave + ":\n    resb 8\n" + File_BSS;


        EntryText = "; global _start\n_start:\n    mov q [" + EntryRSPSave + "], rsp\n    and q rsp, rsp, -16\n    call " + EntryFunction + RetVal + "\n    mov q rsp, [" + EntryRSPSave + "]\n    halt\n"; // TODO Arguments and environment processing
    }

    //
    // Data Assembly
    //

    return File_Header + "; TODO REMOVE AS EXECUTABLE SHOULD PRELOAD RSP\n    mov q rsp, 50000000\n    jmp _start\n; section .bss\n" + File_BSS + "\n; section .data\n" + File_DATA + "\n; section .text\n" + EntryText + File_Text;
}

AssemblyGenerator::AssemblyGenerator(){
    // Setup ; global stack
    this->Stack = new GlobalStack{};

    // Registers

    this->Stack->SymbolMap = {
        {new RegisterDefinition{{{64, "r0"}, {32, "r0"}, {16, "r0"}, {8, "r0"}}}, ""},
        {new RegisterDefinition{{{64, "r1"}, {32, "r1"}, {16, "r1"}, {8, "r1"}}}, ""},
        {new RegisterDefinition{{{64, "r2"}, {32, "r2"}, {16, "r2"}, {8, "r2"}}}, ""},
        {new RegisterDefinition{{{64, "r3"}, {32, "r3"}, {16, "r3"}, {8, "r3"}}}, ""},
        {new RegisterDefinition{{{64, "r4"}, {32, "r4"}, {16, "r4"}, {8, "r4"}}}, ""},
        {new RegisterDefinition{{{64, "r5"}, {32, "r5"}, {16, "r5"}, {8, "r5"}}}, ""},
        {new RegisterDefinition{{{64, "r6"}, {32, "r6"}, {16, "r6"}, {8, "r6"}}}, ""},
        {new RegisterDefinition{{{64, "rax"}, {32, "rax"}, {16, "rax"}, {8, "rax"}}}, ""},
        {new RegisterDefinition{{{64, "rbx"}, {32, "rbx"}, {16, "rbx"}, {8, "rbx"}}}, ""},
        {new RegisterDefinition{{{64, "rcx"}, {32, "rcx"}, {16, "rcx"}, {8, "rcx"}}}, ""},
        {new RegisterDefinition{{{64, "rdx"}, {32, "rdx"}, {16, "rdx"}, {8, "rdx"}}}, ""},
        {new RegisterDefinition{{{64, "rsi"}, {32, "rsi"}, {16, "rsi"}, {8, "rsi"}}}, ""},
        {new RegisterDefinition{{{64, "rdi"}, {32, "rdi"}, {16, "rdi"}, {8, "rdi"}}}, ""},
        {new RegisterDefinition{{{64, "rsp"}, {32, "rsp"}, {16, "rsp"}, {8, "rsp"}}}, ""},
    };
}