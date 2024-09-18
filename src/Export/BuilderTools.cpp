#include <Export/BuilderTools.h>

#include <iostream>
#include <algorithm>

using namespace Exporting;
using namespace Exporting::Helpers;

using namespace Parsing;
using namespace Parsing::SemanticVariables;

extern std::vector<Parsing::LexicalAnalyser*> analysers;
extern std::map<std::string, SemanticTypeDefDeclaration*> StandardisedTypes;
extern std::map<SemanticTypeDefDeclaration*, SemanticObjectDefinition*> StandardObjectTypes;

// TODO File safe tracing!!!!
int Exporting::GetLine(int token){
    if (token < 0){
        return -1;
    }

    for (auto x : analysers){
        if(token >= x->Tokens().size()) return -1;
        return x->getToken(token).fileLine;
    }

    return -1;
}
std::string Exporting::GetLineValue(int token){
    if (token < 0){
        return "No Token";
    }

    for (auto x : analysers){
        if(token >= x->Tokens().size()) return "Token Out of Range";
        return x->getToken(token).tokenFileLine;
    }

    return "";
}

std::vector<std::string> UsableRegisters = {"r0", "r1", "r2", "r3", "r4", "r5", "r6"};

std::string RegisterDefinition::Get(int Size){
    if(Size == 16){
        return this->RegisterTable[128];
    }
    else if(Size == 8){
        return this->RegisterTable[64];
    }
    else if(Size == 4){
        return this->RegisterTable[32];
    }
    else if(Size == 2){
        return this->RegisterTable[16];
    }
    else if(Size == 1){
        return this->RegisterTable[8];
    }
    else{
        return this->RegisterTable[64];
    }

    return "";
}
std::string RegisterDefinition::GetBitOp(int Size){
    if(Size == 16){
        return "q";
    }
    else if(Size == 8){
        return "q";
    }
    else if(Size == 4){
        return "d";
    }
    else if(Size == 2){
        return "w";
    }
    else if(Size == 1){
        return "b";
    }
    else{
        return "q";
    }

    return "";
}

std::string RegisterDefinition::SetVal(SemanticOperation::SemanticOperationValue* Value){
    if(Value->TypeDef == "") throw "No Type Definition for Value";

    if(StandardisedTypes[Value->TypeDef]->DataSize == 8){
        return ("    mov q " + this->RegisterTable[64] + ", " + Value->Value + "\n");
    }
    else if(StandardisedTypes[Value->TypeDef]->DataSize == 4){
        return ("    mov d " + this->RegisterTable[32] + ", " + Value->Value + "\n");
    }
    else if(StandardisedTypes[Value->TypeDef]->DataSize == 2){
        return ("    mov w " + this->RegisterTable[16] + ", " + Value->Value + "\n");
    }
    else if(StandardisedTypes[Value->TypeDef]->DataSize == 1){
        return ("    mov b " + this->RegisterTable[64] + ", " + Value->Value + "\n");
    }
    else{
        return ("    mov q " + this->RegisterTable[64] + " [" + Value->Value + "]\n");
    }
    return "";
}

RegisterDefinition* GlobalStack::GetTempRegister(std::string Symbol, std::string& AssemblyUpdate, std::string UsageID){
    RegisterDefinition* ActiveRegister = nullptr;

    // Get a free register
    for(auto& x : UsableRegisters){
        for(auto& y : SymbolMap){
            if(y.first->RegisterTable[64] == x){
                if(y.second == ""){
                    // Its free
                    ActiveRegister = y.first;
                    y.second = Symbol + "__" + UsageID;
                    break;
                }
            }
        }
        if(ActiveRegister){
            break;
        }
    }

    // Any non-free registers
    if(!ActiveRegister){
        std::cerr << "NO FREE REGISTERS!\n";
        throw 0;
    }

    return ActiveRegister;
}
int GlobalStack::ReleaseTempRegister(std::string Symbol, std::string& AssemblyUpdate){
    for(auto& x : UsableRegisters){
        for(auto& y : SymbolMap){
            if(y.first->RegisterTable[64] == x){
                if(y.second == Symbol){
                    // Its free now
                    y.second = "";
                    return 0;
                }
            }
        }
    }
    return -1;
}
RegisterDefinition* GlobalStack::FindSymbol(std::string Symbol, std::string TypeDef, std::string& AssemblyUpdate){
    return nullptr;
}
std::string GlobalStack::SaveToStack(int Allignment, int Padding){
    return "";
}
std::string GlobalStack::RestoreStack(){
    return "";
}