#pragma once

#include <Parsing/SemanticVariables.h>
#include <Parsing/LexicalAnalyser.h>

#include <list>
#include <map>

namespace Exporting
{
    int GetLine(int token);
    std::string GetLineValue(int line);
    
    namespace Helpers
    {
        struct RegisterDefinition{
            std::map<int, std::string> RegisterTable = {}; // (Bit size to register)
            std::string Get(int Size);
            std::string SetVal(Parsing::SemanticVariables::SemanticOperation::SemanticOperationValue* Value);
        };

        struct GlobalStack{
            std::vector<int> Alters = {};

            std::vector<std::pair<RegisterDefinition*, std::string>> SymbolMap = {};

            RegisterDefinition* GetTempRegister(std::string Symbol, std::string& AssemblyUpdate, std::string UsageID);
            int ReleaseTempRegister(std::string Symbol, std::string& AssemblyUpdate);
            RegisterDefinition* FindSymbol(std::string Symbol, std::string TypeDef, std::string& AssemblyUpdate);
            std::string SaveToStack(int Allignment, int Padding);
            std::string RestoreStack();
        };
    } // namespace SemanticHelpers
} // namespace Exporting
