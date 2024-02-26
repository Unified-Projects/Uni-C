#include <Export/AssemblyGenerator.h>

using namespace Exporting;
using namespace Exporting::Helpers;
using namespace Parsing;
using namespace Parsing::SemanticVariables;

#include <iostream>

extern std::map<std::string, SemanticTypeDefinition> SemanticTypeMap;

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
                    for(auto& Params : ((SemStatement*)Block)->Parameters){
                        if(Params->Type() == SemanticTypeOperation){
                            assembly += ConvertGeneric(Params, IndentCount+1);
                        }
                        else if(Params->Type() == SemanticTypeLiteral){
                            // Check if it is in data (Could be a string not a number) (Use ID to find it)
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
                                    assembly += std::string(IndentCount*4, ' ') + "mov rax, [" + GetSymbol(var) + "]\n";
                                }
                            }
                            else{
                                std::cerr << "Variable Not Found: " << VarName << std::endl;
                            }
                            
                            // Otherwise scope error
                        }
                        else if(Params->Type() == SemanticTypeFunction){
                            assembly += ConvertGeneric(Params);
                        }
                    }
                    assembly += std::string(IndentCount*4, ' ') + "ret\n";
                }
                break;
            
            default:
                break;
            }
        }
        break;
    
    default:
        break;
    }
    return assembly;
}

std::string AssemblyGenerator::Generate(const Parsing::SemanticVariables::SemanticVariable* RootScope){
    // Basic entry assembly that should be consistant
    std::string codeAssembly = "section .text\nglobal _start\n_start:\n    call main\n    mov rdi, rax\n    mov rax, 60\n    syscall\n\nFunctionSpaceStart:\n";
    // std::string codeAssembly = "section .text\nglobal _start\n_start:\n    call main\n    mov ebx, eax\n    mov eax, 1\n    int 0x80\n\nFunctionSpaceStart:\n";
    std::string dataAssembly = "section .data\nDataBlockStart:\n";

    // Generate Scope Tree
    scopeTree->Generate((SemanticVariable*)RootScope);

    // Load Data
    dataAssembly += RecurseTreeForData((SemanticVariable*)RootScope);

    // Load Code
    for(auto& MainScopes : ((Scope*)RootScope)->Block){
        // Each scope here is a head scope and should be handled separately
        for(auto& ScopeContent : ((Scope*)MainScopes)->Block){
            if(ScopeContent->Type() == SemanticTypeFunction){
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

        // Check if it is a string
    }

    return Data;
}

std::string AssemblyGenerator::StoreLiteral(SemLiteral* Lit){
    std::string Data = "";

    // A literal Only Needs to be stored if it is a string

    return Data;
}

std::string AssemblyGenerator::GetSymbol(SemanticVariable* Var){
    std::string Symbol = "";

    if(Var->Type() == SemanticTypeVariable){
        Symbol = ((Variable*)Var)->Identifier + "_" + std::to_string(((Variable*)Var)->LocalScope) + "_" + std::to_string(((Variable*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeFunction){
        Symbol = ((Function*)Var)->Identifier + "_" + std::to_string(((Function*)Var)->LocalScope) + "_" + std::to_string(((Function*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeLiteral){
        Symbol = "Value_" + std::to_string(((SemLiteral*)Var)->LocalScope) + "_" + std::to_string(((SemLiteral*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeScope){
        Symbol = "Scope_" + std::to_string(((Scope*)Var)->LocalScope) + "_" + std::to_string(((Scope*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeType){
        Symbol = ((SemTypeDef*)Var)->Identifier + "_" + std::to_string(((SemTypeDef*)Var)->LocalScope) + "_" + std::to_string(((SemTypeDef*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeStatement){
        Symbol = "Statement_" + std::to_string(((SemStatement*)Var)->TypeID) + "_" + std::to_string(((SemStatement*)Var)->LocalScope) + "_" + std::to_string(((SemStatement*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeOperation){
        Symbol = "Operation_" + std::to_string(((Operation*)Var)->TypeID) + "_" + std::to_string(((Operation*)Var)->LocalScope) + "_" + std::to_string(((Operation*)Var)->ScopePosition);
    }
    else{
        std::cerr << "Unknown Type: " << Var->Type() << std::endl;
    }

    // Recurse Backwards
    if(Var->Parent != nullptr){
        Symbol = GetSymbol(Var->Parent) + "__" + Symbol;
    }

    return Symbol;
}
