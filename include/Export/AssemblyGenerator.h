#pragma once

#include <string>

#include <Parsing/SemanticAnalyser.h>
#include <Export/BuilderTools.h>

namespace Exporting
{
    class AssemblyGenerator
    {
    protected:
        Helpers::GlobalStack* Stack;
        
    public:
        AssemblyGenerator();
        ~AssemblyGenerator() {};
    
    public:
        std::string Generate(Parsing::SemanticAnalyser* Files);
        std::string GenerateBlock(Parsing::SemanticVariables::SemanticisedFile* File, Parsing::SemanticVariables::SemanticFunctionDeclaration* Function, Parsing::SemanticVariables::SemanticBlock* Block);
        std::string InterpretOperation(Parsing::SemanticVariables::SemanticisedFile* File, Parsing::SemanticVariables::SemanticFunctionDeclaration* Function, Parsing::SemanticVariables::SemanticOperation* Operation, Parsing::SemanticVariables::SemanticBlock* ParentBlock);

        std::string GenerateIfTree(Parsing::SemanticVariables::SemanticisedFile* File, Parsing::SemanticVariables::SemanticFunctionDeclaration* Function, Parsing::SemanticVariables::SemanticInstance* s, Parsing::SemanticVariables::SemanticStatment* Statement, Parsing::SemanticVariables::SemanticBlock* Block, std::string Exitter = "", std::string Entry = "");

    protected:
        Parsing::SemanticAnalyser* Files;
        Parsing::SemanticVariables::SemanticisedFile* ActiveFile;
    };
} // namespace Export
