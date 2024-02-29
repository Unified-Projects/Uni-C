#pragma once

#include <string>
#include <vector>
#include <map>

namespace Parsing
{
    struct SemanticTypeAttribute{
        std::string Type = "";
        std::string Name = "";
        std::string Value = "";
    };

    struct SemanticFunctionDeclaration
    {
        std::string Name = "";
        std::string ReturnType = "";
        std::vector<SemanticTypeAttribute> Parameters = {}; // Type, Name
        int Scope = -1;

        class LexicalAnalyser* Lexer = nullptr;
        int TokenIndexOfBlockStart = -1;
        int TokenIndexOfBlockEnd = -1;

        int BlockStart = -1;
        int BlockEnd = -1;
    };

    struct SemanticTypeDefinition{
        std::string Name = "";
        int Scope = -1;
        int Size = -1;
        std::vector<SemanticTypeAttribute> Attributes = {}; // Type, Name

        class LexicalAnalyser* Lexer = nullptr;
        int TokenIndexOfBlockStart = -1;
        int TokenIndexOfBlockEnd = -1;

        int TypeID = -1;

        int BlockStart = -1;
        int BlockEnd = -1;
    };

    namespace SemanticVariables
    {
        enum SemanticTypes{
            SemanticTypeNull,
            SemanticTypeVariable,
            SemanticTypeVariableRef,
            SemanticTypeFunction,
            SemanticTypeType,
            SemanticTypeStatement,
            SemanticTypeOperation,
            SemanticTypeLiteral,
            SemanticTypeScope
        };

        struct SemanticVariable
        {
            int ParentScope = -1;
            int LocalScope = -1;
            int ScopePosition = -1;
            int NextScopePosition = 0;
            SemanticVariable* Parent = nullptr;
            // SemanticTypes Type = SemanticTypes::SemanticTypeNull;
            virtual SemanticTypes Type() {return SemanticTypes::SemanticTypeNull;}
        };

        struct Variable : SemanticVariable
        {
            std::string Identifier = "";
            int TypeID = -1;
            std::string InitValue = "";
            // SemanticTypes Type = SemanticTypes::SemanticTypeVariable;
            virtual SemanticTypes Type() {return SemanticTypes::SemanticTypeVariable;}
        };

        struct VariableRef : SemanticVariable
        {
            std::string Identifier = "";
            // SemanticTypes Type = SemanticTypes::SemanticTypeVariable;
            virtual SemanticTypes Type() {return SemanticTypes::SemanticTypeVariableRef;}
        };

        struct Function : SemanticVariable
        {
            int ReturnTypeID = -1;
            std::string Identifier = "";
            // SemanticTypes Type = SemanticTypes::SemanticTypeFunction;
            virtual SemanticTypes Type() {return SemanticTypes::SemanticTypeFunction;}
            std::vector<Variable*> Parameters = {};
            std::vector<SemanticVariable*> Block = {};
        };

        struct SemTypeDef : SemanticVariable
        {
            std::string Identifier = "";
            int TypeID = -1;
            int Size = -1;
            std::vector<Variable*> Attributes = {};
            std::vector<Function*> Methods = {};
            // SemanticTypes Type = SemanticTypes::SemanticTypeType;
            virtual SemanticTypes Type() {return SemanticTypes::SemanticTypeType;}
        };

        enum SemanticStatementTypes{
            SemanticStatementNULL,
            SemanticStatementIf,
            SemanticStatementWhile,
            SemanticStatementFor,
            SemanticStatementReturn,
            SemanticStatementBreak,
            SemanticStatementContinue,
            SemanticStatementElse,
            SemanticStatementElseIf,
            SemanticStatementSwitch,
            SemanticStatementCase,
            SemanticStatementDefault,
            SemanticStatementNew,
            SemanticStatementDelete
        };

        struct SemStatement : SemanticVariable
        {
            SemanticStatementTypes TypeID = SemanticStatementNULL;
            std::vector<SemanticVariable*> Parameters = {};
            std::vector<SemanticVariable*> Block = {};
            std::vector<SemanticVariable*> Alternatives = {};
            // SemanticTypes Type = SemanticTypes::SemanticTypeStatement;
            virtual SemanticTypes Type() {return SemanticTypes::SemanticTypeStatement;}
        };

        enum SemanticOperationTypes{
            SemanticOperationEqual,
            SemanticOperationNotEqual,
            SemanticOperationGreater,
            SemanticOperationLess,
            SemanticOperationGreaterEqual,
            SemanticOperationLessEqual,
            SemanticOperationAnd,
            SemanticOperationOr,
            SemanticOperationNot,
            SemanticOperationBitwiseAnd,
            SemanticOperationBitwiseOr,
            SemanticOperationBitwiseNot,
            SemanticOperationBitwiseXor,
            SemanticOperationBitwiseLeftShift,
            SemanticOperationBitwiseRightShift,
            SemanticOperationTernary,
            SemanticOperationArrayAccess,
            SemanticOperationMemberAccess,
            SemanticOperationPointerAccess,
            SemanticOperationScoper,
            SemanticOperationAssignment,
            SemanticOperationAddition,
            SemanticOperationSubtraction,
            SemanticOperationMultiplication,
            SemanticOperationDivision,
            SemanticOperationModulus,
            SemanticOperationIncrement,
            SemanticOperationDecrement,
            SemanticOperationFunctionCall,
            SemanticOperationArgument
        };

        struct Operation : SemanticVariable
        {
            int TypeID = -1;
            std::vector<SemanticVariable*> Parameters = {};
            // SemanticTypes Type = SemanticTypes::SemanticTypeOperation;
            virtual SemanticTypes Type() {return SemanticTypes::SemanticTypeOperation;}
        };

        struct SemLiteral : SemanticVariable
        {
            int TypeID = -1;
            std::string Value = "";
            // SemanticTypes Type = SemanticTypes::SemanticTypeLiteral;
            virtual SemanticTypes Type() {return SemanticTypes::SemanticTypeLiteral;}
        };

        struct Scope : SemanticVariable
        {
            std::vector<SemanticVariable*> Block = {};
            // SemanticTypes Type = SemanticTypes::SemanticTypeScope;
            virtual SemanticTypes Type() {return SemanticTypes::SemanticTypeScope;}
        };
        
    } // namespace SemanticVariables
    
} // namespace Parsing
