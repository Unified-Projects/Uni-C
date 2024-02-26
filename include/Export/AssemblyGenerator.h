#pragma once

#include <string>

#include <Parsing/SemanticVariables.h>
#include <Export/BuilderTools.h>

namespace Exporting
{
    class AssemblyGenerator
    {
    protected:
        Helpers::StackTrace* stackTrace = nullptr;
        Helpers::ScopeTree* scopeTree = nullptr;
        Helpers::RegisterTable* registerTable = nullptr;
    public:
        AssemblyGenerator();
        ~AssemblyGenerator();
    
    public:
        std::string Generate(const Parsing::SemanticVariables::SemanticVariable* RootScope);

        std::string ConvertGeneric(Parsing::SemanticVariables::SemanticVariable* Block, int IndentCount = 0);
        std::string ConvertFunction(Parsing::SemanticVariables::Function* Function, int IndentCount = 0);

    public:
        std::string RecurseTreeForData(Parsing::SemanticVariables::SemanticVariable* Block);
        std::string StoreVariable(Parsing::SemanticVariables::Variable* Var);
        std::string StoreLiteral(Parsing::SemanticVariables::SemLiteral* Lit);
    };
} // namespace Export
