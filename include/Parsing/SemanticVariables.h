#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

#include <Parsing/LexicalAnalyser.h>

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
       
        struct SemanticInstance{
            std::string Namespace = "";
            int TokenIndex = 0;
            int TokenSize = 0;
            
            virtual int GetType() {return -1;}
        };

        struct SemanticStatment : SemanticInstance{
            enum StatementType{
                NULL_STATMENT,
                RETURN_STATEMENT,
                STATIC_STATEMENT, // TODO HANDLE CORRECTLY
                CONST_STATEMENT // TODO HANDLE CORRECTLY
            };
            StatementType StateType = StatementType::NULL_STATMENT;

            SemanticInstance* Block = nullptr; // Does not need to be filled

            std::vector<SemanticInstance*> ParameterConditions = {};
            std::vector<SemanticInstance*> ParameterOperations = {};
            std::vector<SemanticVariable*> ParameterVariables = {};

            int GetType() {return 0;}
        };

        struct SemanticOperation : SemanticInstance{
            std::string ResultStore = "";

            struct SemanticOperationValue{
                enum SemanticOperationTypes{
                    OPERATION_NULL,

                    // Oprators
                    OPERATION_VALUE,
                    OPERATION_VARIABLE,
                    OPERATION_FUNCTION,
                    
                    // Actual Operations
                    OPERATION_ADD,
                };
                SemanticOperationTypes Type = SemanticOperationTypes::OPERATION_NULL;
                std::string Value = "";
                std::string TypeDef = "";
                std::vector<SemanticOperation*> Values = {};
            };
            std::vector<SemanticOperationValue*> Operations;

            std::string EvaluatedTypedef = "";
            
            int GetType() {return 1;}
        };

        struct SemanticBlock : SemanticInstance{
            std::vector<SemanticVariable*> Variables = {};
            std::vector<SemanticInstance*> Block = {};
            SemanticBlock* Parent = nullptr; // Return block

            int GetType() {return 2;}
        };

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
            SemanticBlock* Block = nullptr;
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

        struct SemanticisedFile{
            // Metadata
            std::string AssociatedFile = "";
            LexicalAnalyser* Lexar = nullptr;
            std::vector<std::string> Associations = {};
            std::string ID = "";

            // Definitions
            std::vector<SemanticTypeDefDeclaration*> TypeDefs = {}; // (Will load inherited ones from included files)
            std::vector<SemanticObjectDefinition*> ObjectDefs = {};
            std::vector<SemanticFunctionDeclaration*> FunctionDefs = {};

            // Global variables
            std::vector<SemanticVariable*> Variables = {};
        };


    } // namespace SemanticVariables
    
} // namespace Parsing
