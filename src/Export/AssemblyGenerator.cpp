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
            Assembly += "    mov [__" + File->ID + "__" + Function->Symbol + "__RSP + 8], rsp\n    and rsp, -0x40\n";

            Assembly += "    call " + Oper->Value + "\n";

            Assembly += "    mov rsp, [__" + File->ID + "__" + Function->Symbol + "__RSP + 8]\n";

            Assembly += "    push rax\n"; // Load return value
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
                Assembly += "    add " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Types.push(Reg1Type);
            }
            else{
                Assembly += "    add " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg2Type]->DataSize) + "\n";
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
                
                Assembly += "    mov rsp, [__" + File->ID + "__" + Function->Symbol + "__RSP]\n    ret\n";
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
    std::string File_Header = ";; This was automatically generated using the Uni-C Compiler for NASM\nbits 64\ndefault rel\n";
    std::string File_BSS = ""; // Un-Initialised Data (Zeroed)
    std::string File_DATA = ""; // Initialised Data
    std::string File_Text = ""; // Code and Consts

#ifdef WIN32
    std::string File_Externs = "\nextern ExitProcess\n";
#else
#error "UNSUPPORTED OS FORMAT";
#endif

    //
    // Load Global Variables
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
                FunctionString = "global __" + F->ID + "__" + f->Symbol + "\n__" + F->ID + "__" + f->Symbol + ":\n    mov [__" + F->ID + "__" + f->Symbol + "__RSP], rsp\n" + FunctionString;
                File_BSS += "__" + F->ID + "__" + f->Symbol + "__RSP:\n    resb 16\n";

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
                            RetVal = "\n    mov rax, 0";
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


        EntryText = "global _start\n_start:\n    mov [" + EntryRSPSave + "], rsp\n    and rsp, -16\n    call " + EntryFunction + RetVal + "\n    mov rsp, [" + EntryRSPSave + "]\n    ret\n"; // TODO Arguments and environment processing
    }

    //
    // Data Assembly
    //

    return File_Header + File_Externs + "\nsection .bss\n" + File_BSS + "\nsection .data\n" + File_DATA + "\nsection .text\n" + EntryText + File_Text;
}

AssemblyGenerator::AssemblyGenerator(){
    // Setup global stack
    this->Stack = new GlobalStack{};

    // Registers
    // R8-15 are Generic and usable
    // RAX - Return value
    // RSP - Stack reserve
    // RBX-RDX RSI,RDI,RBP - Argument (WinAPI uses RCX, RDX, R8, R9)

    // XMM0-XMM15 - FLOAT

    this->Stack->SymbolMap = {
        {new RegisterDefinition{{{64, "r8"}, {32, "r8d"}, {16, "r8w"}, {8, "r8b"}, {128, "xmm0"}}}, ""},
        {new RegisterDefinition{{{64, "r9"}, {32, "r9d"}, {16, "r9w"}, {8, "r9b"}, {128, "xmmo"}}}, ""},
        {new RegisterDefinition{{{64, "r10"}, {32, "r10d"}, {16, "r10w"}, {8, "r10b"}, {128, "xmmo"}}}, ""},
        {new RegisterDefinition{{{64, "r11"}, {32, "r11d"}, {16, "r11w"}, {8, "r11b"}, {128, "xmmo"}}}, ""},
        {new RegisterDefinition{{{64, "r12"}, {32, "r12d"}, {16, "r12w"}, {8, "r12b"}, {128, "xmmo"}}}, ""},
        {new RegisterDefinition{{{64, "r13"}, {32, "r13d"}, {16, "r13w"}, {8, "r13b"}, {128, "xmmo"}}}, ""},
        {new RegisterDefinition{{{64, "r14"}, {32, "r14d"}, {16, "r14w"}, {8, "r14b"}, {128, "xmmo"}}}, ""},
        {new RegisterDefinition{{{64, "r15"}, {32, "r15d"}, {16, "r15w"}, {8, "r15b"}, {128, "xmmo"}}}, ""},
        {new RegisterDefinition{{{64, "rax"}, {32, "eax"}, {16, "ax"}, {8, "rax ERR Require movb"}, {128, ""}}}, ""},
        {new RegisterDefinition{{{64, "rsp"}, {32, "esp"}, {16, "sp"}, {8, "rsp ERR Require movb"}, {128, ""}}}, ""},
        {new RegisterDefinition{{{64, "rbx"}, {32, "ebx"}, {16, "bx"}, {8, "rbx ERR Require movb"}, {128, ""}}}, ""},
        {new RegisterDefinition{{{64, "rcx"}, {32, "ecx"}, {16, "cx"}, {8, "rcx ERR Require movb"}, {128, ""}}}, ""},
        {new RegisterDefinition{{{64, "rdx"}, {32, "edx"}, {16, "dx"}, {8, "rdx ERR Require movb"}, {128, ""}}}, ""},
        {new RegisterDefinition{{{64, "rsi"}, {32, "esi"}, {16, "si"}, {8, "rsi ERR Require movb"}, {128, ""}}}, ""},
        {new RegisterDefinition{{{64, "rdi"}, {32, "edi"}, {16, "di"}, {8, "rdi ERR Require movb"}, {128, ""}}}, ""},
        {new RegisterDefinition{{{64, "rbp"}, {32, "ebp"}, {16, "bp"}, {8, "rbp ERR Require movb"}, {128, ""}}}, ""}
    };
}