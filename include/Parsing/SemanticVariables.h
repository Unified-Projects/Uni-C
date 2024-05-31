#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace Parsing
{
    namespace SemanticVariables
    {
        /*
        PLAN:

        WHAT IS NEEDED

        SemanticSpace:
            - Include Files (Analyse First)
            - Associated File
            - All defined TypeDefs
            - All defined Classes
            - All defined functions

        Declarations
        TypeDef:
            - Identifier
            - Size
            - Compound
            - Reference
        
        Obj:
            - Inherited TypeDef
            - Deconstruction of Size {In TypeDef's}
            - Associated Fucntion Names

        Function:
            - Identifier
            - {Does Return, Type}
            - {Parameters}
            - SystemLevel
            - Namespace -> (Default is __GLOB__)
            - Symbol
            - Static (Defines is callable without class scope)


        SemanticGeneric:
            - Type() Function to get type
            - int ReferenceToken
            - Parent

        SemanticFunction:
            - [] of SemanticVariables and their token position of definition
        */

        struct SemanticTypeDefDeclaration{
            std::string Identifier = "";
            uint64_t DataSize = 0;
            bool Compound = false;
            struct {
                bool Referenceing;
                std::string RelayTypeDef = ""; 
            } Refering;
            std::string Namepsace = "__GLOB__";
        };

        struct SemanticFunctionDeclaration{
            std::string Identifier = "";
            struct {
                bool WillReturn = false;
                std::string TypeDef = "";
            } FunctionReturn;
            struct FunctionDefinitionParameter{
                std::string TypeDef = "";
                std::string Identifier = "";
                std::string Initialiser = "";
            };
            std::vector<FunctionDefinitionParameter*> Parameters = {};
            bool IsBuiltin = false;
            std::string Namespace = "";
            std::string Symbol = "";
            bool Static = false;
            bool Private = false;
            int StartToken = -1;
            int EndToken = -1;
        };

        struct SemanticVariable{
            bool Private = false;
            bool Protected = false;
            bool Constant = false;
            bool Static = false;
            bool Pointer = false;
            std::string TypeDef = "";
            std::string Identifier = ""; // (Need to backtrack scopes to see if we can create)
            std::string Initialiser = ""; // TODO SUPPORT {} Initialisers for CLASSES (Verify they match typedef)
            std::string Namespace = ""; // Basically Scope (Each __X__ acts as a back tracable scope)
            int DefinedToken = 0;
        };

        struct SemanticObjectDefinition{
            std::string TypeDefinition = "";
            std::vector<SemanticVariable*> Variables = {}; // (Appeneded to File aswell)
            std::vector<SemanticFunctionDeclaration*> AssociatedFunctions = {};
            std::string Inhertance = "";
            std::string Namespace = "__GLOB__";
            int StartToken = -1;
            int EndToken = -1;
        };

        struct SemanticInstance{
            std::string Namespace = "";
            int TokenIndex = 0;
            int TokenSize = 0;
            
            virtual int GetType() {return -1;}
        };

        struct SemanticStatment{
            enum StatementType{
                NULL_STATMENT,
                RETURN_STATEMENT,
                STATIC_STATEMENT, // TODO HANDLE CORRECTLY
                CONST_STATEMENT // TODO HANDLE CORRECTLY
            } StateType = StatementType::NULL_STATMENT;

            SemanticInstance* Block = nullptr; // Does not need to be filled

            SemanticInstance* ParameterCondition = nullptr;
            SemanticInstance* ParameterOperation = nullptr;
            std::vector<SemanticVariable*> ParameterVariables = {};

            virtual int GetType() {return 0;}
        };

        struct SemanticOperation{
            SemanticVariable* ResultStore = nullptr;

            class SemanticOperationValue{
                enum SemanticOperationTypes{
                    OPERATION_VALUE,
                    OPERATION_VARIABLE
                };
            };
            std::vector<SemanticOperationValue*> Operations;
            
            virtual int GetType() {return 1;}
        };

        struct SemanticBlock : SemanticInstance{
            std::vector<SemanticVariable*> Variables;
            std::vector<SemanticInstance*> Block;

            virtual int GetType() {return 2;}
        };

        struct SemanticisedFile{
            // Metadata
            std::string AssociatedFile = "";
            LexicalAnalyser* Lexar = nullptr;
            std::vector<std::string> Associations = {};

            // Definitions
            std::vector<SemanticTypeDefDeclaration*> TypeDefs = {}; // (Will load inherited ones from included files)
            std::vector<SemanticObjectDefinition*> ObjectDefs = {};
            std::vector<SemanticFunctionDeclaration*> FunctionDefs = {};

            // Global variables
            std::vector<SemanticVariable*> Variables = {};

            // Interpreted Blocks
        };


    } // namespace SemanticVariables
    
} // namespace Parsing
