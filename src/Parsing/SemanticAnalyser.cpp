#include <Parsing/SemanticAnalyser.h>

using namespace Parsing;
using namespace Parsing::SemanticVariables;

#include <stack> // RPN
#include <algorithm>

#include <iostream> // Used for error logging

std::map<int, std::string> SemanticFunctionArgs{
    // ArgNumber : register
    {0, "rbx"},
    {1, "rcx"},
    {2, "rdx"},
    {3, "rsi"},
    {4, "rdi"},
    // TODO Continue into stack....
};

//TODO Finish all
// std::vector<std::string> StandardTypeDefs = {
//     "int",
//     "uint",
//     "short",
//     "ushort",
//     "long",
//     "ulong",
//     "float",
//     "double",
//     "char",
//     "uchar",
//     "bool",
//     "void",
//     "string"
// };

// TODO UPDATE TYPEDEF CHECK TO CHECK NAMESPACE COMPATIBILITY!!!!!!
// The whole namespacing of types needs to work
// If not defined in a parent but rather parent-sub like within a class accept use of Class1::Class2
// Basically to forward track ensure we use Namespace::Namespace and ensure token allocations match!!!!!

extern std::string GenerateID(); // Unique-ID Generator

std::map<std::string, SemanticTypeDefDeclaration*> StandardisedTypes{
    {"int", new SemanticTypeDefDeclaration{"int", 4, false, {false, ""}, "__GLOB__"}},
    {"uint", new SemanticTypeDefDeclaration{"uint", 4, false, {false, ""}, "__GLOB__"}},
    {"short", new SemanticTypeDefDeclaration{"short", 2, false, {false, ""}, "__GLOB__"}},
    {"ushort", new SemanticTypeDefDeclaration{"ushort", 2, false, {false, ""}, "__GLOB__"}},
    {"long", new SemanticTypeDefDeclaration{"long", 8, false, {false, ""}, "__GLOB__"}},
    {"ulong", new SemanticTypeDefDeclaration{"ulong", 8, false, {false, ""}, "__GLOB__"}},
    {"", new SemanticTypeDefDeclaration{"", 4, false, {false, ""}, "__GLOB__"}},
    {"", new SemanticTypeDefDeclaration{"", 4, false, {false, ""}, "__GLOB__"}},
    {"char", new SemanticTypeDefDeclaration{"char", 1, false, {false, ""}, "__GLOB__"}},
    {"uchar", new SemanticTypeDefDeclaration{"uchar", 1, false, {false, ""}, "__GLOB__"}},
    {"", new SemanticTypeDefDeclaration{"", 4, false, {false, ""}, "__GLOB__"}},
    {"", new SemanticTypeDefDeclaration{"", 4, false, {false, ""}, "__GLOB__"}},
};
std::map<SemanticTypeDefDeclaration*, SemanticObjectDefinition*> StandardObjectTypes{
    {new SemanticTypeDefDeclaration{"string", 12, false, {false, ""}, "__GLOB__"}, new SemanticObjectDefinition{"string", {new SemanticVariable{true, false, false, false, false, "ulong", "Length", "0", "__GLOB____CLASS____string__", -1}, new SemanticVariable{true, false, false, false, true, "char", "Data", "0", "__GLOB____CLASS____string__", -1}}, {/*TODO Things like Size, Substr, FirstOf...*/}, "__GLOB__", "", -1, -1}} // String
};

std::map<std::string, std::vector<std::string>> TypeConversionCompatibilityMap{ // Todo Match operations that are defined to allow custom type conversions too
    {"int", {"int", "uint", "short", "ushort", "long", "ulong", "char", "uchar"}},
    {"uint", {"int", "uint", "short", "ushort", "long", "ulong", "char", "uchar"}},
    {"short", {"int", "uint", "short", "ushort", "long", "ulong", "char", "uchar"}},
    {"ushort", {"int", "uint", "short", "ushort", "long", "ulong", "char", "uchar"}},
    {"long", {"int", "uint", "short", "ushort", "long", "ulong", "char", "uchar"}},
    {"ulong", {"int", "uint", "short", "ushort", "long", "ulong", "char", "uchar"}},
    {"", {}},
    {"", {}},
    {"char", {"int", "uint", "short", "ushort", "long", "ulong", "char", "uchar"}},
    {"uchar", {"int", "uint", "short", "ushort", "long", "ulong", "char", "uchar"}},
    {"", {}},
    {"", {}},
    {"string", {}}
};

std::map<std::string, SemanticOperation::SemanticOperationValue::SemanticOperationTypes> OperationTypeMap{
    {"+", SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_ADD}, 
    {"-", SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_SUB}, 
    {"/", SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_DIV}, 
    {"*", SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_MUL}, 
};

std::map<std::string, SemanticCondition::ConditionTypes> ConditionTypeMap{
    {"==", SemanticCondition::ConditionTypes::EQU_CONDITION},  
    {"!=", SemanticCondition::ConditionTypes::NEQ_CONDITION},  
    {"<=", SemanticCondition::ConditionTypes::LEQ_CONDITION},  
    {"<", SemanticCondition::ConditionTypes::LTH_CONDITION},  
    {">", SemanticCondition::ConditionTypes::GTH_CONDITION},  
    {">=", SemanticCondition::ConditionTypes::GEQ_CONDITION},  
};

std::map<std::string, SemanticBooleanOperator::ConditionOperationTypes> BooleanOperationTypeMap{
    {"&&", SemanticBooleanOperator::AND_CONDITION_OPERATION},  
    {"||", SemanticBooleanOperator::OR_CONDITION_OPERATION},  
};

namespace Classifiers{ // Functions that are used to classify types of things like statements and operations
    int FindObjectVariableDefinitions(SemanticisedFile* File, SemanticObjectDefinition* Obj, int StartI, int EndI, std::string NameSpace){
        LexicalAnalyser* Lexar = File->Lexar;

        for(int i = StartI; i < Lexar->Tokens().size() && (i < EndI || EndI == -1);){ // Manual Incremenation
            // Skip Embed Blocks
            if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart){
                i++; // '{'
                int EndNeeded = 1;
                while(EndNeeded != 0 && Lexar->getToken(i).tokenType != TokenTypes::NullToken){
                    if(Lexar->getToken(i).tokenType == TokenTypes::BlockEnd){
                        EndNeeded--;
                    }
                    else if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart){
                        EndNeeded++;
                    }

                    i++;
                }
                if(Lexar->getToken(i).tokenType == TokenTypes::NullToken){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'}' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }
            }

            if(Lexar->getToken(i).tokenType == TokenTypes::LineEnd){
                i++;
                continue;
            }

            // Load Types
            // Should go (c/s)|("") (p/P)|("") (TypeDef / Identifier) (Identifier) ("")|("=" "(Value)") ;
            
            // Config Values
            bool Private = false;
            bool Protected = false;
            bool Constant = false;
            bool Static = false;
            bool Pointer = false;

            std::string TypeDef = "";
            std::string Identifier = "";
            std::string InitialValue = "";

            // First check for Static/Const (Can be skipped)
            if(Lexar->getToken(i).tokenType == TokenTypes::Statement){
                if(Lexar->getToken(i).tokenValue == "const"){
                    Constant = true;
                }
                else if(Lexar->getToken(i).tokenValue == "static"){
                    Static = true;
                }
                else if(Lexar->getToken(i).tokenValue == "protected"){
                    Protected = true;
                }
                else if(Lexar->getToken(i).tokenValue == "private"){
                    Private = true;
                }
                else{
                    i++;
                    continue; // Not valid
                }

                i++; // Skip it
            }

            if(Lexar->getToken(i).tokenType == TokenTypes::Statement && (Constant || Static)) { // Could have protected or private after
                if(Lexar->getToken(i).tokenValue == "protected"){
                    Protected = true;
                }
                else if(Lexar->getToken(i).tokenValue == "private"){
                    Private = true;
                }
                else{
                    i++;
                    continue; // Not valid
                }

                i++; // Skip it
            }

            // Type
            int DefToken = i;
            if(Lexar->getToken(i).tokenType == TokenTypes::TypeDef){
                // Its a standard type
                TypeDef = Lexar->getToken(i).tokenValue;
                i++;
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::Identifier){
                // See if we defined it yet
                for(auto x : File->TypeDefs){
                    if(x->Identifier == Lexar->getToken(i).tokenValue){
                        TypeDef = Lexar->getToken(i).tokenValue;
                        break;
                    }
                }

                if(TypeDef.size() <= 0){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Type not defined " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }

                i++;
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Type Expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                return -1;
            }

            // See if this is a pointer
            if(Lexar->getToken(i).tokenType == TokenTypes::Operator && Lexar->getToken(i).tokenValue == "*"){
                // We are pointer
                Pointer =  true;
                i++;
            }

            // Identifier
            if(Lexar->getToken(i).tokenType == TokenTypes::Identifier){
                // See if we defined it yet
                for(auto x : File->TypeDefs){
                    if(x->Identifier == Lexar->getToken(i).tokenValue){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Cannot use type as identifier " << Lexar->getToken(i).tokenValue << std::endl;
                        return -1;
                    }
                }

                Identifier = Lexar->getToken(i).tokenValue;

                i++;
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart && Lexar->getToken(i-2).tokenType == TokenTypes::Statement){
                continue; // Could be a subdefined struct
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Identifier Expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                return -1;
            }

            // Initial value?
            if(Lexar->getToken(i).tokenType == TokenTypes::Operator && Lexar->getToken(i).tokenValue == "="){
                // Assigning a default value
                i++;
                if(Lexar->getToken(i).tokenType == TokenTypes::Literal && Lexar->getToken(i+1).tokenType == TokenTypes::LineEnd){
                    InitialValue = Lexar->getToken(i).tokenValue;
                    i+=2;
                }
                else{ // TODO Macros able to load here
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected Literal Initial Valuebut got " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::LineEnd){
                // No Initial Value
                i++;
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::ArgumentStart){ // Function within the struct
                int EndNeeded = 0;

                while(Lexar->getToken(i).tokenType != TokenTypes::BlockStart && Lexar->getToken(i).tokenType != TokenTypes::LineEnd){
                    i++;
                    if(Lexar->getToken(i).tokenType == TokenTypes::NullToken){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected '{' or ';' but got " << Lexar->getToken(i).tokenValue << std::endl;
                        return -1;
                    }
                }

                EndNeeded += (Lexar->getToken(i).tokenType == TokenTypes::BlockStart);
                i++;

                while(EndNeeded != 0 && Lexar->getToken(i).tokenType != TokenTypes::NullToken){
                    if(Lexar->getToken(i).tokenType == TokenTypes::BlockEnd){
                        EndNeeded--;
                    }
                    else if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart){
                        EndNeeded++;
                    }

                    i++;
                }

                if(Lexar->getToken(i).tokenType == TokenTypes::NullToken && EndNeeded > 0){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'}' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }
                continue; // Skip the function declaration
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "';' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                return -1;
            }

            // If we got here we can create the component
            SemanticVariable* ObjComp = new SemanticVariable{};
            ObjComp->Private = Private;
            ObjComp->Protected = Protected;
            ObjComp->Constant = Constant;
            ObjComp->Static = Static;
            ObjComp->Pointer = Pointer;
            ObjComp->TypeDef = TypeDef;
            ObjComp->Identifier = Identifier;
            ObjComp->Initialiser = InitialValue;
            ObjComp->Namespace = Obj->Namespace + "__CLASS____" + Obj->TypeDefinition + "__";
            ObjComp->DefinedToken = DefToken;

            Obj->Variables.push_back(ObjComp);
            File->Variables.push_back(ObjComp); // Also part of main file (So can be traced)
        }

        return 0;
    }

    int FindTypeDeclarations(SemanticisedFile* File, int StartI = 0, int EndI = -1, std::string NameSpace = "__GLOB__"){
        LexicalAnalyser* Lexar = File->Lexar;

        // Start of looking at typedefs
        for(int i = StartI; i < Lexar->Tokens().size() && (i < EndI || EndI == -1);){ // Manual Incremenation
            // All start with a statement
            if(Lexar->getToken(i).tokenType == TokenTypes::Statement){
                // See if it is a "using" (A redirect)
                if(Lexar->getToken(i).tokenValue == "using"){
                    i++; // "using"

                    // First is new type
                    Token NewIdentifier = Lexar->getToken(i);
                    i++;
                    if (NewIdentifier.tokenType == TokenTypes::TypeDef){
                        std::cerr << File->AssociatedFile << ":" << NewIdentifier.fileLine << " << Semantic::" << __LINE__ << " << " << "Cannot re-assign standard types" << std::endl;
                        return -1;
                    }
                    if(NewIdentifier.tokenType != TokenTypes::Identifier){
                        std::cerr << File->AssociatedFile << ":" << NewIdentifier.fileLine << " << Semantic::" << __LINE__ << " << " << "Identifier cannot be used" << std::endl;
                        return -1;
                    }

                    // See if we already defined it
                    for(auto x : File->TypeDefs){
                        if(x->Identifier == NewIdentifier.tokenValue && (x->Namepsace == NameSpace || x->Namepsace == "__GLOB__")){
                            std::cerr << File->AssociatedFile << ":" << NewIdentifier.fileLine << " << Semantic::" << __LINE__ << " << " << "Identifier is already a TypeDef" << std::endl;
                            return -1;
                        }
                    }

                    // Get refering
                    Token RedirectId = Lexar->getToken(i);
                    i++;

                    // See if we defined it
                    SemanticTypeDefDeclaration* Relayed = nullptr;
                    for(auto x : File->TypeDefs){
                        if(x->Identifier == RedirectId.tokenValue && (x->Namepsace == NameSpace || x->Namepsace == "__GLOB__")){
                            Relayed = x;
                        }
                    }
                    if(Relayed){
                        SemanticTypeDefDeclaration* TypeDefDec = new SemanticTypeDefDeclaration{};
                        TypeDefDec->Identifier = NewIdentifier.tokenValue;
                        TypeDefDec->DataSize = Relayed->DataSize;
                        TypeDefDec->Compound = Relayed->Compound;
                        TypeDefDec->Refering.Referenceing = true;
                        TypeDefDec->Refering.RelayTypeDef = RedirectId.tokenValue;

                        File->TypeDefs.push_back(TypeDefDec);
                    }

                    if (RedirectId.tokenType == TokenTypes::TypeDef){
                        SemanticTypeDefDeclaration* TypeDefDec = new SemanticTypeDefDeclaration{};
                        TypeDefDec->Identifier = NewIdentifier.tokenValue;
                        TypeDefDec->DataSize = StandardisedTypes[RedirectId.tokenValue]->DataSize; // TODO: Work out relayed size of standards
                        TypeDefDec->Compound = StandardisedTypes[RedirectId.tokenValue]->Compound; // TODO: Depends if it is a string or not (Once again look at standards)
                        TypeDefDec->Refering.Referenceing = true;
                        TypeDefDec->Refering.RelayTypeDef = RedirectId.tokenValue;

                        File->TypeDefs.push_back(TypeDefDec);
                    }else{
                        std::cerr << File->AssociatedFile << ":" << NewIdentifier.fileLine << " << Semantic::" << __LINE__ << " << " << "No type defined " << NewIdentifier.tokenValue << std::endl;
                        return -1;
                    }

                    if(Lexar->getToken(i++).tokenType != TokenTypes::LineEnd){
                        std::cerr << File->AssociatedFile << ":" << NewIdentifier.fileLine << " << Semantic::" << __LINE__ << " << " << "';' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                        return -1;
                    }
                    continue;
                }
                else if(Lexar->getToken(i).tokenValue == "struct" || Lexar->getToken(i).tokenValue == "class"){
                    i++; // struct/class
                    // New typedef
                    SemanticTypeDefDeclaration* TypeDefDec = new SemanticTypeDefDeclaration{};
                    SemanticObjectDefinition* ObjectDef = new SemanticObjectDefinition{};

                    // TODO: Interpret Inheritance

                    // Find token name
                    if(Lexar->getToken(i).tokenType != TokenTypes::Identifier){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Identifier is either a TypeDef or non-Identifier (" << Lexar->getToken(i).tokenValue << ")" << std::endl;
                        return -1;
                    }

                    // Already in use?
                    for(auto x : File->TypeDefs){
                        if(x->Identifier == Lexar->getToken(i).tokenValue && (x->Namepsace == NameSpace || x->Namepsace == "__GLOB__")){
                            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Identifier already used as a typedef" << std::endl;
                            return -1;
                        }
                    }

                    std::string Ident = Lexar->getToken(i++).tokenValue;

                    // Interpret the block size
                    if(Lexar->getToken(i).tokenType != TokenTypes::BlockStart){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'{' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                        return -1;
                    }
                    i++; // '{'

                    int TokenStart = i;
                    int EndNeeded = 1;

                    while(EndNeeded != 0 && Lexar->getToken(i).tokenType != TokenTypes::NullToken){
                        if(Lexar->getToken(i).tokenType == TokenTypes::BlockEnd){
                            EndNeeded--;
                        }
                        else if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart){
                            EndNeeded++;
                        }

                        i++;
                    }

                    if(Lexar->getToken(i).tokenType == TokenTypes::NullToken){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'}' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                        return -1;
                    }
                    
                    int TokenEnd = i - 2;

                    if(Lexar->getToken(i).tokenType != TokenTypes::LineEnd){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "';' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                        return -1;
                    }
                    i++;

                    // Now see if any are defined within
                    FindTypeDeclarations(File, TokenStart, TokenEnd, NameSpace + "__Class____" + Ident + "__");

                    // Setup TypeDec
                    TypeDefDec->Identifier = Ident;
                    TypeDefDec->DataSize = 0;
                    TypeDefDec->Compound = true;
                    TypeDefDec->Refering.Referenceing = false;
                    TypeDefDec->Namepsace = NameSpace;

                    // Load to file
                    File->TypeDefs.push_back(TypeDefDec); // Relied on for Parenting as we setup variables post

                    // Setup Object Declaration
                    ObjectDef->TypeDefinition = Ident;
                    ObjectDef->Inhertance = ""; // TODO Inheritance
                    ObjectDef->Namespace = NameSpace;
                    ObjectDef->StartToken = TokenStart;
                    ObjectDef->EndToken = TokenEnd;

                    if(FindObjectVariableDefinitions(File, ObjectDef, TokenStart, TokenEnd, NameSpace + "__Class____" + Ident + "__")){
                        return -1;
                    }

                    // Update Size of TypeDefDec
                    for(auto C : ObjectDef->Variables){
                        // Pointer
                        if (C->Pointer){
                            TypeDefDec->DataSize += 8; // Fixed uint64_t pointer size
                            continue;
                        }

                        // Then see if its part of defined ones
                        bool Found = false;
                        for(auto D : File->TypeDefs){
                            if(D->Identifier == C->TypeDef){
                                TypeDefDec->DataSize += D->DataSize;
                                Found = true;
                            }
                        }

                        if(Found)
                            continue;

                        // Then it must be standard
                        TypeDefDec->DataSize += StandardisedTypes[C->TypeDef]->DataSize;
                    }

                    File->ObjectDefs.push_back(ObjectDef);

                    continue;
                }
            }

            i++;
        }

        return 0;
    }

    int FindFunctionParameters(SemanticisedFile* File, SemanticFunctionDeclaration* Func, int StartI = 0, int EndI = -1, std::string NameSpace = "__GLOB__"){
        LexicalAnalyser* Lexar = File->Lexar;

        bool StartedInitialValues = false;

        // Start index should be first parameter!
        for(int i = StartI; i < Lexar->Tokens().size() && (i < EndI || EndI == -1);){
            std::string TypeDef = "";
            std::string Identifier = "";
            std::string InitialValue = "";

            // Type
            if(Lexar->getToken(i).tokenType == TokenTypes::TypeDef){
                // Its a standard type
                TypeDef = Lexar->getToken(i).tokenValue;
                i++;
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::Identifier){
                // See if we defined it yet
                for(auto x : File->TypeDefs){
                    if(x->Identifier == Lexar->getToken(i).tokenValue){
                        TypeDef = Lexar->getToken(i).tokenValue;
                        break;
                    }
                }

                if(TypeDef.size() <= 0){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Type not defined " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }

                i++;
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Type Expected" << std::endl;
                return -1;
            }

            // Identifier
            if(Lexar->getToken(i).tokenType == TokenTypes::Identifier){
                // See if we defined it yet
                for(auto x : File->TypeDefs){
                    if(x->Identifier == Lexar->getToken(i).tokenValue){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Cannot use type as identifier " << Lexar->getToken(i).tokenValue << std::endl;
                        return -1;
                    }
                }

                Identifier = Lexar->getToken(i).tokenValue;

                i++;
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Identifier Expected" << std::endl;
                return -1;
            }

            // Initial value?
            if(Lexar->getToken(i).tokenType == TokenTypes::Operator && Lexar->getToken(i).tokenValue == "="){
                // Assigning a default value
                i++;
                if(Lexar->getToken(i).tokenType == TokenTypes::Literal){
                    InitialValue = Lexar->getToken(i).tokenValue;
                    i++;
                }
                else{ // TODO Macros able to load here
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected Literal Initial Value" << std::endl;
                    return -1;
                }

                StartedInitialValues = true;
            }
            else if((Lexar->getToken(i).tokenType == TokenTypes::ArgumentEnd || Lexar->getToken(i).tokenType == TokenTypes::ArgumentSeparator) && !StartedInitialValues){
                // No Initial Value
                i++;
            }
            else if(StartedInitialValues){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected Initial Value but got " << Lexar->getToken(i).tokenValue << std::endl;
                return -1;
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "')' or ',' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                return -1;
            }

            // Construct Type
            SemanticFunctionDeclaration::FunctionDefinitionParameter* FuncParam = new SemanticFunctionDeclaration::FunctionDefinitionParameter{};
            FuncParam->Identifier = Identifier;
            FuncParam->Initialiser = InitialValue;
            FuncParam->TypeDef = TypeDef;

            Func->Parameters.push_back(FuncParam);
        }

        return 0;
    }

    int FindFunctions(SemanticisedFile* File, int StartI = 0, int EndI = -1, std::string NameSpace = "__GLOB__"){

        LexicalAnalyser* Lexar = File->Lexar;

        // Cannot be defined in blocks other than NameSpaces, Classes/Structs and Globaly

        for(int i = StartI; i < Lexar->Tokens().size() && (i < EndI || EndI == -1);){
            if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart){
                // Find what opens a block here

                // TODO NAMESPACES
                // TODO Block Indents

                int IBefore = i;

                // Classes / Structs
                for(auto c : File->ObjectDefs){
                    if (c->StartToken == i + 1){
                        // We are at the begining of this block
                        FindFunctions(File, c->StartToken, c->EndToken, c->Namespace + "__CLASS____" + c->TypeDefinition + "__");
                        i += c->EndToken - c->StartToken;
                        break;
                    }
                }

                if(i == IBefore){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Function cannot be defined within another block" << std::endl;
                    return -1;
                }

                i+=2; // Skip both "{" and "}"
            }

            if(Lexar->getToken(i).tokenType != TokenTypes::TypeDef && Lexar->getToken(i).tokenType != TokenTypes::Identifier && Lexar->getToken(i).tokenType != TokenTypes::Statement){
                i++;
                continue;
            }

            std::string TypeDef = "";
            std::string FunctionName = "";
            bool Static = false;
            bool Private = false;
            int FunctionStartToken = -1;
            int FunctionEndToken = -1;

            if(Lexar->getToken(i).tokenType == TokenTypes::Statement){
                // We should only see Using or Struct/Class
                if(Lexar->getToken(i).tokenValue == "struct" || Lexar->getToken(i).tokenValue == "class"){
                    i+=2; // Skip struct and identifier (Rest is auto handled ^)
                    continue;
                }
                else if(Lexar->getToken(i).tokenValue == "using"){
                    i+=4;
                    continue;
                }
                else if(Lexar->getToken(i).tokenValue == "static"){
                    Static = true;
                    i++;
                }
                else if(Lexar->getToken(i).tokenValue == "private"){
                    Private = true;
                    i++;
                }
                else{
                    i++;
                    continue;
                }
            }

            /// Type
            if(Lexar->getToken(i).tokenType == TokenTypes::TypeDef){
                // Its a standard type
                TypeDef = Lexar->getToken(i).tokenValue;
                i++;
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::Identifier){
                // See if we defined it yet
                for(auto x : File->TypeDefs){
                    if(x->Identifier == Lexar->getToken(i).tokenValue){
                        TypeDef = Lexar->getToken(i).tokenValue;
                        break;
                    }
                }

                if(TypeDef.size() <= 0){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Type not defined " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }

                i++;
            }
            else{
                
            }

            if(Lexar->getToken(i).tokenValue == "*"){
                // Most likeley global variable
                while(Lexar->getToken(i).tokenType != TokenTypes::LineEnd && Lexar->getToken(i).tokenType != TokenTypes::NullToken){
                    i++;
                }

                if(Lexar->getToken(i).tokenType == TokenTypes::NullToken){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "';' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }

                i++;
                continue;
            }

            // Find function name
            if(Lexar->getToken(i).tokenType != TokenTypes::Identifier){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Identifier is either a TypeDef or non-Identifier (" << Lexar->getToken(i).tokenValue << ")" << std::endl;
                return -1;
            }

            // Already in use?
            for(auto x : File->TypeDefs){
                if(x->Identifier == Lexar->getToken(i).tokenValue && (x->Namepsace == NameSpace || x->Namepsace == "__GLOB__")){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Identifier already used as a typedef" << std::endl;
                    return -1;
                }
            }

            FunctionName = Lexar->getToken(i++).tokenValue;

            // Could be global variable
            if(Lexar->getToken(i).tokenType == TokenTypes::Operator || Lexar->getToken(i).tokenType == TokenTypes::LineEnd){
                // Most likeley global variable
                while(Lexar->getToken(i).tokenType != TokenTypes::LineEnd && Lexar->getToken(i).tokenType != TokenTypes::NullToken){
                    i++;
                }

                if(Lexar->getToken(i).tokenType == TokenTypes::NullToken){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "';' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }

                i++;
                continue;
            }

            // Arguments
            int StartArgument, ArgumentEnd = 0;
            if(Lexar->getToken(i).tokenType == TokenTypes::ArgumentStart){
                // Find end argument
                StartArgument = i+1;

                // Most likeley global variable
                while(Lexar->getToken(i).tokenType != TokenTypes::ArgumentEnd && Lexar->getToken(i).tokenType != TokenTypes::NullToken){
                    i++;
                }

                ArgumentEnd = i-1;

                i++;

                if(Lexar->getToken(i).tokenType == TokenTypes::NullToken){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "')' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }
            }

            // Block data
            if(Lexar->getToken(i).tokenType == TokenTypes::LineEnd){
                // See if it is defined
                for(auto F : File->FunctionDefs){
                    if(F->Symbol == NameSpace + "__" + FunctionName + "__"){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << F->Symbol << " is already defined" << std::endl;
                        return -1;
                    }
                }

                // End of function definition
                SemanticFunctionDeclaration* FuncDef = new SemanticFunctionDeclaration{};
                FuncDef->Identifier = FunctionName;
                FuncDef->FunctionReturn.WillReturn = (TypeDef != "void");
                FuncDef->FunctionReturn.TypeDef = TypeDef;
                FuncDef->Parameters = {};
                FuncDef->IsBuiltin = false; // TODO Lookup in a table to check (As this is where it is important)
                FuncDef->Namespace = NameSpace;
                FuncDef->Symbol = NameSpace + "____FUNC____" + FunctionName + "__";
                FuncDef->Static = Static;
                FuncDef->Private = Private;
                FuncDef->StartToken = FunctionStartToken;
                FuncDef->EndToken = FunctionEndToken;
                FuncDef->Block = nullptr;

                // Load Function Parameters;
                if(FindFunctionParameters(File, FuncDef, StartArgument, ArgumentEnd, NameSpace)){
                    return -1;
                }
                if(FuncDef->Parameters.size() > 6){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << FuncDef->Identifier << " has too many parameters" << std::endl;
                    return -1;
                }

                File->FunctionDefs.push_back(FuncDef);

                i++;
                continue;
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart){
                // TODO COULD DEFINE A ASSOCIATED FILE

                i++;

                // Find end Block
                int TokenStart = i;
                int EndNeeded = 1;

                while(EndNeeded != 0 && Lexar->getToken(i).tokenType != TokenTypes::NullToken){
                    if(Lexar->getToken(i).tokenType == TokenTypes::BlockEnd){
                        EndNeeded--;
                    }
                    else if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart){
                        EndNeeded++;
                    }

                    i++;
                }

                if(Lexar->getToken(i).tokenType == TokenTypes::NullToken && Lexar->getToken(i-1).tokenType != TokenTypes::BlockEnd){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'}' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }
                
                int TokenEnd = i - 1;

                FunctionStartToken = TokenStart;
                FunctionEndToken = TokenEnd;
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'{' or ';' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
            }

            SemanticFunctionDeclaration* FuncDef = new SemanticFunctionDeclaration{};
            File->FunctionDefs.push_back(FuncDef);
            
            // See if it is defined
            for(auto F : File->FunctionDefs){
                if(F->Symbol == NameSpace + "____FUNC____" + FunctionName + "__"){
                    if(F->StartToken < 0){
                        // TODO VERIFY PARAMETER COUNT AND TYPES!!!!!!!! ALONGSIDE PRIV, STATIC

                        // Its not defined
                        File->FunctionDefs.pop_back();
                        delete FuncDef;
                        FuncDef = F;
                        break;
                    }

                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << F->Symbol << "Already defined" << std::endl;
                    return -1;
                }
            }

            // Load function values
            FuncDef->Identifier = FunctionName;
            FuncDef->FunctionReturn.WillReturn = (TypeDef != "void");
            FuncDef->FunctionReturn.TypeDef = TypeDef;
            // FuncDef->Parameters = {};
            FuncDef->IsBuiltin = false; // TODO Lookup in a table to check (As this is where it is important)
            FuncDef->Namespace = NameSpace;
            FuncDef->Symbol = NameSpace + "___FUNC___" + FunctionName + "__";
            FuncDef->Static = Static;
            FuncDef->Private = Private;
            FuncDef->StartToken = FunctionStartToken;
            FuncDef->EndToken = FunctionEndToken;
            FuncDef->Block = nullptr;

            if(FuncDef->Parameters.size() == 0){ // TODO Make sure the functions paramenters match when "redefining"
                // Load Function Parameters
                if(FindFunctionParameters(File, FuncDef, StartArgument, ArgumentEnd, NameSpace)){
                    return -1;
                }
                if(FuncDef->Parameters.size() > 6){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << FuncDef->Identifier << " has too many parameters" << std::endl;
                    return -1;
                }
            }

            if(NameSpace.find_first_of("__CLASS__") != NameSpace.npos){
                // Its part of a class
                for(auto C : File->ObjectDefs){
                    if(C->Namespace + "__CLASS____" + C->TypeDefinition + "__" == NameSpace){
                        C->AssociatedFunctions.push_back(FuncDef);
                        break;
                    }
                }
            }
        }

        return 0;
    }

    int FindGlobalVariables(SemanticisedFile* File){
        LexicalAnalyser* Lexar = File->Lexar;

        for(int i = 0; i < Lexar->Tokens().size();){
            if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart){
                int EndNeeded = 1;

                i++;

                while(EndNeeded != 0 && Lexar->getToken(i).tokenType != TokenTypes::NullToken){
                    if(Lexar->getToken(i).tokenType == TokenTypes::BlockEnd){
                        EndNeeded--;
                    }
                    else if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart){
                        EndNeeded++;
                    }

                    i++;
                }

                if(Lexar->getToken(i).tokenType == TokenTypes::LineEnd){
                    i++;
                }

                continue;
            } // Skip all blocks

            // Config Values
            bool Private = false;
            bool Protected = false;
            bool Constant = false;
            bool Static = false;
            bool Pointer = false;

            std::string TypeDef = "";
            std::string Identifier = "";
            std::string InitialValue = "";

            if(Lexar->getToken(i).tokenType == TokenTypes::Statement){
                if(Lexar->getToken(i).tokenValue == "const"){
                    Constant = true;
                }
                else if(Lexar->getToken(i).tokenValue == "static"){
                    Static = true;
                }
                else if(Lexar->getToken(i).tokenValue == "protected"){
                    Protected = true;
                }
                else if(Lexar->getToken(i).tokenValue == "private"){
                    Private = true;
                }
                else if(Lexar->getToken(i).tokenValue == "struct" || Lexar->getToken(i).tokenValue == "class"){
                    i+=2; // Skip struct and identifier (Rest is auto handled ^)
                    continue;
                }
                else if(Lexar->getToken(i).tokenValue == "using"){
                    i+=4;
                    continue;
                }
                else{
                    i++;
                    continue; // Not valid
                }

                i++; // Skip it
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::Macro){
                i += 3;
                continue;
            }

            // Is there typedef / Identifier
            // Type
            int DefToken = i;
            if(Lexar->getToken(i).tokenType == TokenTypes::TypeDef){
                // Its a standard type
                TypeDef = Lexar->getToken(i).tokenValue;
                i++;
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::Identifier){
                // See if we defined it yet
                for(auto x : File->TypeDefs){
                    if(x->Identifier == Lexar->getToken(i).tokenValue){
                        TypeDef = Lexar->getToken(i).tokenValue;
                        break;
                    }
                }

                if(TypeDef.size() <= 0){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Type not defined " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }

                i++;
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Type Expected (" << Lexar->getToken(i).tokenValue << ")" << std::endl;
                return -1;
            }

            // See if this is a pointer
            if(Lexar->getToken(i).tokenType == TokenTypes::Operator && Lexar->getToken(i).tokenValue == "*"){
                // We are pointer
                Pointer =  true;
                i++;
            }

            // Identifier
            if(Lexar->getToken(i).tokenType == TokenTypes::Identifier){
                // See if we defined it yet
                for(auto x : File->TypeDefs){
                    if(x->Identifier == Lexar->getToken(i).tokenValue){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Cannot use type as identifier " << Lexar->getToken(i).tokenValue << std::endl;
                        return -1;
                    }
                }

                Identifier = Lexar->getToken(i).tokenValue;

                i++;
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart && Lexar->getToken(i-2).tokenType == TokenTypes::Statement){
                continue; // Could be a subdefined struct
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Identifier Expected (" << Lexar->getToken(i).tokenValue << ")" << std::endl;
                return -1;
            }

            // Initial value?
            if(Lexar->getToken(i).tokenType == TokenTypes::Operator && Lexar->getToken(i).tokenValue == "="){
                // Assigning a default value
                i++;
                if(Lexar->getToken(i).tokenType == TokenTypes::Literal && Lexar->getToken(i+1).tokenType == TokenTypes::LineEnd){
                    InitialValue = Lexar->getToken(i).tokenValue;
                    i+=2;
                }
                else{ // TODO Macros able to load here
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected Literal Initial Value" << std::endl;
                    return -1;
                }
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::LineEnd){
                // No Initial Value
                i++;
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::ArgumentStart){ // Function within the struct
                int EndNeeded = 0;

                while(Lexar->getToken(i).tokenType != TokenTypes::BlockStart && Lexar->getToken(i).tokenType != TokenTypes::LineEnd){
                    i++;
                    if(Lexar->getToken(i).tokenType == TokenTypes::NullToken){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected '{' or ';'" << std::endl;
                        return -1;
                    }
                }

                EndNeeded += (Lexar->getToken(i).tokenType == TokenTypes::BlockStart);
                i++;

                while(EndNeeded != 0 && Lexar->getToken(i).tokenType != TokenTypes::NullToken){
                    if(Lexar->getToken(i).tokenType == TokenTypes::BlockEnd){
                        EndNeeded--;
                    }
                    else if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart){
                        EndNeeded++;
                    }

                    i++;
                }

                if(Lexar->getToken(i).tokenType == TokenTypes::NullToken && EndNeeded > 0){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'}' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                    return -1;
                }
                continue; // Skip the function declaration
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "';' expected but got " << Lexar->getToken(i).tokenValue << std::endl;
                return -1;
            }

            // If we got here we can create the component
            SemanticVariable* GlobVar = new SemanticVariable{};
            GlobVar->Private = Private;
            GlobVar->Protected = Protected;
            GlobVar->Constant = Constant;
            GlobVar->Static = Static;
            GlobVar->Pointer = Pointer;
            GlobVar->TypeDef = TypeDef;
            GlobVar->Identifier = Identifier;
            GlobVar->Initialiser = InitialValue;
            GlobVar->Namespace = "__GLOB__";
            GlobVar->DefinedToken = DefToken;

            File->Variables.push_back(GlobVar); // Also part of main file (So can be traced)
        }

        return 0;
    }

    int InterpretVariableDefinition(SemanticisedFile* File, SemanticFunctionDeclaration* Function, SemanticBlock* ParentBlock, int& TokenIndex){
        LexicalAnalyser* Lexar = File->Lexar;

        // Config Values
        bool Private = false;
        bool Protected = false;
        bool Constant = false;
        bool Static = false;
        bool Pointer = false;

        std::string TypeDef = "";
        std::string Identifier = "";
        std::string InitialValue = "";

        if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Statement){
            if(Lexar->getToken(TokenIndex).tokenValue == "const"){
                Constant = true;
            }
            else if(Lexar->getToken(TokenIndex).tokenValue == "static"){
                Static = true;
            }
            else if(Lexar->getToken(TokenIndex).tokenValue == "protected"){
                Protected = true;
            }
            else if(Lexar->getToken(TokenIndex).tokenValue == "private"){
                Private = true;
            }
            else if(Lexar->getToken(TokenIndex).tokenValue == "struct" || Lexar->getToken(TokenIndex).tokenValue == "class"){
                TokenIndex+=2; // Skip struct and identifier (Rest is auto handled ^)
                return -1;
            }
            else if(Lexar->getToken(TokenIndex).tokenValue == "using"){
                TokenIndex+=4;
                return -1;
            }
            else{
                TokenIndex++;
                return -1;
            }

            TokenIndex++; // Skip it
        }
        else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Macro){
            TokenIndex += 3;
            return -1;
        }

        // Is there typedef / Identifier
        // Type
        int DefToken = TokenIndex;
        if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::TypeDef){
            // Its a standard type
            TypeDef = Lexar->getToken(TokenIndex).tokenValue;
            TokenIndex++;
        }
        else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Identifier){
            // See if we defined it yet
            for(auto x : File->TypeDefs){
                if(x->Identifier == Lexar->getToken(TokenIndex).tokenValue){
                    TypeDef = Lexar->getToken(TokenIndex).tokenValue;
                    return -1;
                }
            }

            if(TypeDef.size() <= 0){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Type not defined " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }

            TokenIndex++;
        }
        else{
            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Type Expected (" << Lexar->getToken(TokenIndex).tokenValue << ")" << std::endl;
            return -1;
        }

        // See if this is a pointer
        if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Operator && Lexar->getToken(TokenIndex).tokenValue == "*"){
            // We are pointer
            Pointer =  true;
            TokenIndex++;
        }

        // Identifier
        if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Identifier){
            // See if we defined it yet
            for(auto x : File->TypeDefs){
                if(x->Identifier == Lexar->getToken(TokenIndex).tokenValue){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Cannot use type as identifier " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                    return -1;
                }
            }

            Identifier = Lexar->getToken(TokenIndex).tokenValue;

            TokenIndex++;
        }
        else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::BlockStart && Lexar->getToken(TokenIndex-2).tokenType == TokenTypes::Statement){
            return -1;
        }
        else{
            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Identifier Expected (" << Lexar->getToken(TokenIndex).tokenValue << ")" << std::endl;
            return -1;
        }

        // Initial value?
        if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Operator && Lexar->getToken(TokenIndex).tokenValue == "="){
            // Assigning a default value
            TokenIndex++;
            if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Literal && Lexar->getToken(TokenIndex+1).tokenType == TokenTypes::LineEnd){
                InitialValue = Lexar->getToken(TokenIndex).tokenValue;
                TokenIndex+=2;
            }
            else{ // TODO Macros able to load here
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected Literal Initial Value" << std::endl;
                return -1;
            }
        }
        else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::LineEnd){
            // No Initial Value
            TokenIndex++;
        }
        else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::ArgumentStart){ // Function within the struct
            int EndNeeded = 0;

            while(Lexar->getToken(TokenIndex).tokenType != TokenTypes::BlockStart && Lexar->getToken(TokenIndex).tokenType != TokenTypes::LineEnd){
                TokenIndex++;
                if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::NullToken){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected '{' or ';'" << std::endl;
                    return -1;
                }
            }

            EndNeeded += (Lexar->getToken(TokenIndex).tokenType == TokenTypes::BlockStart);
            TokenIndex++;

            while(EndNeeded != 0 && Lexar->getToken(TokenIndex).tokenType != TokenTypes::NullToken){
                if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::BlockEnd){
                    EndNeeded--;
                }
                else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::BlockStart){
                    EndNeeded++;
                }

                TokenIndex++;
            }

            if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::NullToken && EndNeeded > 0){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "'}' expected but got " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            return -1;
        }
        else{
            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "';' expected but got " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
            return -1;
        }

        // If we got here we can create the component
        SemanticVariable* GlobVar = new SemanticVariable{};
        GlobVar->Private = Private;
        GlobVar->Protected = Protected;
        GlobVar->Constant = Constant;
        GlobVar->Static = Static;
        GlobVar->Pointer = Pointer;
        GlobVar->TypeDef = TypeDef;
        GlobVar->Identifier = Identifier;
        GlobVar->Initialiser = InitialValue;
        GlobVar->Namespace = ParentBlock->Namespace;
        GlobVar->DefinedToken = DefToken;

        ParentBlock->Variables.push_back(GlobVar); // Also part of main file (So can be traced)

        return 0;
    }

    int InterpretOpration(SemanticisedFile* File, SemanticFunctionDeclaration* Function, SemanticBlock* ParentBlock, SemanticOperation*& RetPoint, int& TokenIndex, bool function = false, int EndToken = -1){
        // Ensure typedef consistancy
        LexicalAnalyser* Lexar = File->Lexar;

        int StartI = TokenIndex;
        
        SemanticOperation* Operation = new SemanticOperation{};
        Operation->Namespace = ParentBlock->Namespace;

        std::stack<SemanticOperation::SemanticOperationValue*> Operations = {};
        std::stack<int> BracketIndex = {};
        int ValuesLoaded = 0;

        int Brackets = 1;
        
        while(((Lexar->getToken(TokenIndex).tokenType != TokenTypes::LineEnd && !function) || ((Lexar->getToken(TokenIndex).tokenType != TokenTypes::ArgumentEnd || Brackets > 1) && Lexar->getToken(TokenIndex).tokenType != TokenTypes::ArgumentSeparator && function)) && (TokenIndex <= EndToken || EndToken == -1)){
            SemanticOperation::SemanticOperationValue* Oper = new SemanticOperation::SemanticOperationValue{};

            if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Literal){
                Oper->Value = Lexar->getToken(TokenIndex).tokenValue;
                Oper->Type = SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VALUE;
                // TODO Custom Intialisers
                if(Oper->Value.find_first_not_of("0123456789-") == std::string::npos && ('-' == Oper->Value.front() || Oper->Value.find('-') == std::string::npos) && std::count(Oper->Value.begin(), Oper->Value.end(), '-') <= 1){    
                    long Value = std::atol(Oper->Value.data());
                    if(Value <= 0xFF / 2){
                        Oper->TypeDef = "char";
                    }
                    else if (Value <= 0xFFFF / 2){
                        Oper->TypeDef = "short";
                    }
                    else if(Value <= 0xFFFFFFFF / 2){
                        Oper->TypeDef = "int";
                    }
                    else{
                        Oper->TypeDef = "long";
                    }
                }
                else if(Oper->Value.find_first_not_of("0123456789-.") == std::string::npos && ('-' == Oper->Value.front() || Oper->Value.find('-') == std::string::npos) && std::count(Oper->Value.begin(), Oper->Value.end(), '-') <= 1 && std::count(Oper->Value.begin(), Oper->Value.end(), '.') <= 1){
                    Oper->TypeDef = "double";
                }
                else if(Oper->Value == std::string("true") || Oper->Value == std::string("false")){
                    Oper->TypeDef = "bool";
                }
                else if(Oper->Value[0] == '\"' && Oper->Value.ends_with("\"")){
                    Oper->TypeDef = "string";
                }
                else if(Oper->Value[0] == '\'' && Oper->Value.ends_with("'")){
                    Oper->TypeDef = "char";
                }
                else{
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Encountered Invalid Literal" << std::endl;
                    return -1;
                }

                Operation->Operations.push_back(Oper);
                ValuesLoaded++;
            }
            else if (Lexar->getToken(TokenIndex).tokenType == TokenTypes::Operator){
                auto OperationType = OperationTypeMap[Lexar->getToken(TokenIndex).tokenValue];

                while(Operations.size() > 0 && Operations.top()->Type > OperationType){
                    // More preferable
                    Operation->Operations.push_back(Operations.top());
                    Operations.pop();
                    ValuesLoaded-=1;
                }

                // Load current operation to stack
                Oper->Type = OperationType;
                Operations.push(Oper);
            }
            else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Identifier){
                // Variable // (Check)
                // TODO Variables in operations

                // Function
                if(Lexar->getToken(TokenIndex+1).tokenType == TokenTypes::ArgumentStart){
                    // Check function is defined
                    for(auto Functions : File->FunctionDefs){
                        // TODO Check NAMEPACE TRACKING
                        if(Lexar->getToken(TokenIndex).tokenValue == Functions->Identifier){
                            // Load the rest
                            Oper->Type = SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_FUNCTION;

                            if(!Functions->FunctionReturn.WillReturn){
                                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Function does not return a value (" << Functions->Identifier << ")" << std::endl;
                                return -1;
                            }

                            TokenIndex+=2;

                            while(Lexar->getToken(TokenIndex).tokenType != TokenTypes::ArgumentEnd){
                                if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::LineEnd || TokenIndex >= ParentBlock->TokenIndex + ParentBlock->TokenSize){
                                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Function operator arguments invalid" << std::endl;
                                    return -1;
                                }

                                SemanticOperation* ParamOper =  nullptr;

                                if(InterpretOpration(File, Function, ParentBlock, ParamOper, TokenIndex, true)){
                                    return -1;
                                }

                                // Registers
                                ParamOper->ResultStore = SemanticFunctionArgs[Oper->Values.size()];
                                Oper->Values.push_back(ParamOper);
                            }

                            int RequiredParams = 0;
                            int MaxParams = 0;
                            for(auto p : Functions->Parameters){
                                if(p->Initialiser == ""){
                                    RequiredParams++;
                                }
                                MaxParams++;
                            }
                            if(Oper->Values.size() < RequiredParams || Oper->Value.size() > MaxParams){
                                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Invalid parameter count" << std::endl;
                                return -1;
                            }

                            for(int p = 0; p < Functions->Parameters.size(); p++){
                                if(Functions->Parameters[p]->Initialiser == ""){
                                    bool Valid = false;
                                    for(auto x : TypeConversionCompatibilityMap[Oper->Values[p]->EvaluatedTypedef]){
                                        if(x == Functions->Parameters[p]->TypeDef){
                                            // We can convert
                                            Valid = true;
                                            break;
                                        }
                                    }
                                    if(!Valid){
                                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Cannot convert from " << Oper->Values[p]->EvaluatedTypedef << " to " << Functions->Parameters[p]->TypeDef << std::endl;
                                        return -1;
                                    }
                                }
                                else{
                                    if(Oper->Values.size() > p){
                                        bool Valid = false;
                                        for(auto x : TypeConversionCompatibilityMap[Oper->Values[p]->EvaluatedTypedef]){
                                            if(x == Functions->Parameters[p]->TypeDef){
                                                // We can convert
                                                Valid = true;
                                                break;
                                            }
                                        }
                                        if(!Valid){
                                            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Cannot convert from " << Oper->Values[p]->EvaluatedTypedef << " to " << Functions->Parameters[p]->TypeDef << std::endl;
                                            return -1;
                                        }
                                    }
                                    else{
                                        SemanticOperation* ParameterOperation = new SemanticOperation{};
                                        ParameterOperation->EvaluatedTypedef = Functions->Parameters[p]->TypeDef;
                                        ParameterOperation->Namespace = Operation->Namespace;
                                        ParameterOperation->ResultStore = SemanticFunctionArgs[p];
                                        ParameterOperation->TokenIndex = -1;
                                        ParameterOperation->TokenSize = 0;
                                        SemanticOperation::SemanticOperationValue* LoadParam = new SemanticOperation::SemanticOperationValue{};
                                        LoadParam->Value = Functions->Parameters[p]->Initialiser;
                                        LoadParam->Type = SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VALUE;
                                        // TODO Custom Intialisers
                                        if(LoadParam->Value.find_first_not_of("0123456789-") == std::string::npos && ('-' == LoadParam->Value.front() || LoadParam->Value.find('-') == std::string::npos) && std::count(LoadParam->Value.begin(), LoadParam->Value.end(), '-') <= 1){    
                                            long Value = std::atol(LoadParam->Value.data());
                                            if(Value <= 0xFF / 2){
                                                LoadParam->TypeDef = "char";
                                            }
                                            else if (Value <= 0xFFFF / 2){
                                                LoadParam->TypeDef = "short";
                                            }
                                            else if(Value <= 0xFFFFFFFF / 2){
                                                LoadParam->TypeDef = "int";
                                            }
                                            else{
                                                LoadParam->TypeDef = "long";
                                            }
                                        }
                                        else if(LoadParam->Value.find_first_not_of("0123456789-.") == std::string::npos && ('-' == LoadParam->Value.front() || LoadParam->Value.find('-') == std::string::npos) && std::count(LoadParam->Value.begin(), LoadParam->Value.end(), '-') <= 1 && std::count(LoadParam->Value.begin(), Oper->Value.end(), '.') <= 1){
                                            LoadParam->TypeDef = "double";
                                        }
                                        else if(LoadParam->Value == std::string("true") || LoadParam->Value == std::string("false")){
                                            LoadParam->TypeDef = "bool";
                                        }
                                        else if(LoadParam->Value[0] == '\"' && LoadParam->Value.ends_with("\"")){
                                            LoadParam->TypeDef = "string";
                                        }
                                        else if(LoadParam->Value[0] == '\'' && LoadParam->Value.ends_with("'")){
                                            LoadParam->TypeDef = "char";
                                        }
                                        else{
                                            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Encountered Invalid Literal" << std::endl;
                                            return -1;
                                        }

                                        ParameterOperation->Operations.push_back(LoadParam);
                                        Oper->Values.push_back(ParameterOperation);
                                    }
                                }
                            }

                            Oper->TypeDef = Functions->FunctionReturn.TypeDef;
                            Oper->Value = Functions->Symbol;

                            Operation->Operations.push_back(Oper);
                            ValuesLoaded++;

                            break;
                        }
                    }

                    if(Lexar->getToken(TokenIndex).tokenType != TokenTypes::ArgumentEnd){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "')' Expected but got(" + Lexar->getToken(TokenIndex).tokenValue << ")" << std::endl;
                        return -1;
                    }
                }
                else{ // Just a variable Identifier
                    Oper->Value = Lexar->getToken(TokenIndex).tokenValue;
                    Oper->Type = SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VARIABLE;
                    
                    bool Global, Local = false;
                    int i = 0;
                    // Check if its local
                    for(auto x : Function->Parameters){
                        if(x->Identifier == Oper->Value){
                            Oper->TypeDef = x->TypeDef;
                            Oper->Type = SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VARIABLE_ARGUMENT;
                            Oper->Value = std::to_string(i);
                            Local = true;
                        }
                        i++;
                    }
                    if(!Local){
                        std::stack<Parsing::SemanticVariables::SemanticBlock *> blockStack;
                        blockStack.push(ParentBlock);

                        while (!blockStack.empty()) {
                            Parsing::SemanticVariables::SemanticBlock * currentBlock = blockStack.top();
                            blockStack.pop();

                            for (auto x : currentBlock->Variables) {
                                if (x->Identifier == Oper->Value){
                                    Local = true;
                                    Oper->Value = currentBlock->Namespace + "__" + x->Identifier;
                                    Oper->TypeDef = x->TypeDef;
                                    break; // Assuming you only need the first match
                                }
                            }

                            if (currentBlock->Parent == Function->Block) {
                                break; // STOP RECURSING
                            }

                            if (currentBlock->Parent) {
                                blockStack.push(currentBlock->Parent);
                            }
                        }
                    }
                    // Is it global
                    // TODO Load Global Variable Typdefs

                    if(!Global && !Local){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Non-Global(//TODO) or Local Variable Used (" + Lexar->getToken(TokenIndex).tokenValue << ")" << std::endl;
                    }

                    Operation->Operations.push_back(Oper);
                    ValuesLoaded++;
                }
            }
            else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::ArgumentStart){
                Brackets++;
                BracketIndex.push(Operations.size());
            }
            else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::ArgumentEnd){
                Brackets--;
                int OperationsToPush = Operations.size() - BracketIndex.top();
                BracketIndex.pop();

                for(; OperationsToPush > 0; OperationsToPush--){
                    Operation->Operations.push_back(Operations.top());
                    Operations.pop();
                    ValuesLoaded--;
                }
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Non-Implemented Operation capability Found (" + Lexar->getToken(TokenIndex).tokenValue << ")" << std::endl;
                return -1;
            }

            TokenIndex++;
            if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::NullToken){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Encountered EOF while interpreting operation" << std::endl;
                return -1;
            }
        }

        // Load remaning operations from stack
        while(Operations.size() > 0){
            // More preferable
            Operation->Operations.push_back(Operations.top());
            Operations.pop();
            ValuesLoaded-=1;
        }

        if(ValuesLoaded != 1){
            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Operator to operations missmatch " << ValuesLoaded << std::endl;
            return -1;
        }

        // Evalue typedef
        // Prioritsise larger data types
        std::string EvalType = "";
        for(auto o : Operation->Operations){
            if(StandardisedTypes[o->TypeDef] == nullptr){
                std::cerr << "Custom types are currentyl unsupported for operations" << std::endl;
                return -1;
            }

            if(o->Type != SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VALUE && o->Type != SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VARIABLE && o->Type != SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VARIABLE_ARGUMENT){
                continue;
            }

            // if(o->Type <= SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_ADD){
                if(EvalType == ""){
                    EvalType = o->TypeDef;
                }
                else{
                    if(StandardisedTypes[EvalType]->DataSize < StandardisedTypes[o->TypeDef]->DataSize){
                        bool Valid = false;
                        for(auto x : TypeConversionCompatibilityMap[EvalType]){
                            if(x == o->TypeDef){
                                // We can convert
                                Valid = true;
                                break;
                            }
                        }
                        if(Valid){
                            EvalType = o->TypeDef;
                        }
                        else{
                            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Typedef missmatch for operator" << std::endl;
                            return -1;
                        }
                    }
                    else{
                        bool Valid = false;
                        for(auto x : TypeConversionCompatibilityMap[o->TypeDef]){
                            if(x == EvalType){
                                // We can convert
                                Valid = true;
                                break;
                            }
                        }
                        if(!Valid){
                            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Typedef missmatch for operator" << std::endl;
                            return -1;
                        }
                    }
                }
            // }
        }
        Operation->EvaluatedTypedef = EvalType;

        RetPoint = Operation;

        return 0;
    }

    int InterpretBlock(SemanticisedFile* File, SemanticFunctionDeclaration* Function, SemanticBlock* ParentBlock, int& TokenIndex);

    int InterpretStatement(SemanticisedFile* File, SemanticFunctionDeclaration* Function, SemanticBlock* ParentBlock, int& TokenIndex){
        LexicalAnalyser* Lexar = File->Lexar;

        if(Lexar->getToken(TokenIndex).tokenValue == "return"){ // Return statement
            SemanticStatment* SemStatement = new SemanticStatment{};
            SemStatement->StateType = SemanticStatment::StatementType::RETURN_STATEMENT;
            SemStatement->Namespace = ParentBlock->Namespace;
            SemStatement->TokenIndex = TokenIndex;

            TokenIndex++;

            SemanticOperation* Operation = nullptr;
            
            // Operation
            if(Lexar->getToken(TokenIndex).tokenType != TokenTypes::LineEnd && Function->FunctionReturn.WillReturn){    
                if(InterpretOpration(File, Function, ParentBlock, Operation, TokenIndex)){
                    return -1;
                }

                SemStatement->ParameterOperations.push_back(Operation);

                // Ensure type consistency
                if(Operation->EvaluatedTypedef != Function->FunctionReturn.TypeDef){
                    bool Valid = false;
                    std::cout << Operation->EvaluatedTypedef << std::endl;
                    for(auto x : TypeConversionCompatibilityMap[Operation->EvaluatedTypedef]){
                        if(x == Function->FunctionReturn.TypeDef){
                            // We can convert
                            Valid = true;
                            break;
                        }
                    }
                    if(!Valid){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Attempted to return non " << Function->FunctionReturn.TypeDef << std::endl;
                        return -1;
                    }

                    Operation->EvaluatedTypedef = Function->FunctionReturn.TypeDef;
                }

                Operation->ResultStore = "rax";
            }
            else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::LineEnd && Function->FunctionReturn.WillReturn){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Attempted to return with no value" << std::endl;
                return -1;
            }

            SemStatement->TokenSize = TokenIndex - SemStatement->TokenIndex;

            if(Lexar->getToken(TokenIndex).tokenType != TokenTypes::LineEnd){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "';' expected but got " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }

            TokenIndex++;

            ParentBlock->Block.push_back(SemStatement);

            return 0;
        }
        else if (Lexar->getToken(TokenIndex).tokenValue == "if"){
            TokenIndex++;

            if(Lexar->getToken(TokenIndex).tokenType != TokenTypes::ArgumentStart){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected '(' but got " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            TokenIndex++;

            SemanticStatment* SemStatement = new SemanticStatment{};
            SemStatement->StateType = SemanticStatment::StatementType::IF_STATEMENT;
            SemStatement->Namespace = ParentBlock->Namespace;
            SemStatement->TokenIndex = TokenIndex - 2;

            int ArguemntsToEnd = 1;

            SemanticCondition* CurrentOperation = nullptr;
            SemanticOperation* Argument1 = nullptr;

            std::stack<SemanticStatment*> CurrentOperatingStatement = {};
            CurrentOperatingStatement.push(SemStatement);

            // Basically condition bidamas
            while(Lexar->getToken(TokenIndex).tokenType != TokenTypes::NullToken && ArguemntsToEnd > 0){
                if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Comparitor){
                    CurrentOperation = new SemanticCondition();
                    CurrentOperation->Condition = ConditionTypeMap[Lexar->getToken(TokenIndex).tokenValue];
                    CurrentOperation->Namespace = ParentBlock->Namespace;
                    CurrentOperation->TokenIndex = TokenIndex - 2;
                    CurrentOperation->Operation1 = Argument1;
                    Argument1 = nullptr;
                    TokenIndex++;
                    continue;
                }
                else if(Lexar->getToken(TokenIndex).tokenType == BooleanOperator){
                    // Combinations of Conditions
                    if(CurrentOperation || Argument1){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected Boolean Operator within condition " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                        return -1;
                    }

                    SemanticBooleanOperator* BoolOp = new SemanticBooleanOperator();
                    BoolOp->Condition = BooleanOperationTypeMap[Lexar->getToken(TokenIndex).tokenValue];
                    TokenIndex++;
                    CurrentOperatingStatement.top()->ParameterConditions.push_back(BoolOp);

                    continue;
                }
                else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::ArgumentStart){
                    // Only if it contains a comparison otherwise its a operation
                    int ClonedTokenIndex = TokenIndex;
                    bool Process = false;
                    int BlocksToEscape = 0;
                    while(BlocksToEscape >= 0 && Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::NullToken){
                        if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::Comparitor){
                            Process = true;
                            break;
                        }
                        if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::BooleanOperator){
                            Process = true;
                            break;
                        }
                        else if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentStart){
                            BlocksToEscape++;
                        }
                        else if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentEnd){
                            BlocksToEscape--;
                        }
                        ClonedTokenIndex++;
                    }
                    if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::NullToken){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                        return -1;
                    }

                    if(Process){
                        // BracketsStack.push(ConditionStack.size());
                        // Start building a new condition
                        auto newStatement = new SemanticStatment();
                        newStatement->StateType = SemanticStatment::StatementType::BOOLEAN_STATEMENT;
                        newStatement->Namespace = ParentBlock->Namespace + "____" + GenerateID() + "__";
                        newStatement->TokenIndex = TokenIndex;
                        CurrentOperatingStatement.push(newStatement);

                        ArguemntsToEnd++;
                        TokenIndex++;
                        continue;
                    }
                }
                else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::ArgumentEnd){
                    ArguemntsToEnd--;

                    auto Top = CurrentOperatingStatement.top();
                    CurrentOperatingStatement.pop();

                    if(Top != SemStatement){
                        CurrentOperatingStatement.top()->ParameterConditions.push_back(Top);
                    }

                    TokenIndex++;
                    continue;
                }

                // Otherwise interpret as a operation
                // Find end of operation
                int ClonedTokenIndex = TokenIndex;
                int EndToken = TokenIndex - 1;
                int BlocksToEscape = 0;
                while(Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::Comparitor && Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::BooleanOperator && (Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::ArgumentEnd || BlocksToEscape) && Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::NullToken){
                    ClonedTokenIndex++;
                    EndToken++;
                    if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentStart){
                        BlocksToEscape++;
                    }
                    else if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentEnd && BlocksToEscape){
                        BlocksToEscape--;
                    }
                }
                if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::NullToken){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(ClonedTokenIndex).tokenValue << std::endl;
                    return -1;
                }

                // Interpret
                if(!CurrentOperation){
                    InterpretOpration(File, Function, ParentBlock, Argument1, TokenIndex, false, EndToken);
                }
                else{
                    InterpretOpration(File, Function, ParentBlock, CurrentOperation->Operation2, TokenIndex, false, EndToken);
                    CurrentOperatingStatement.top()->ParameterConditions.push_back(CurrentOperation);
                    CurrentOperation = nullptr;
                }

            }

            if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::NullToken){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }

            if(Lexar->getToken(TokenIndex).tokenType != TokenTypes::BlockStart){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected '{' Got " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            TokenIndex++;
            SemanticBlock* Bloc = new SemanticBlock();
            SemStatement->Block = Bloc;
            Bloc->TokenIndex = TokenIndex;
            Bloc->Namespace = SemStatement->Namespace + "__IF____" + GenerateID() + "__";
            Bloc->Parent = ParentBlock;
            
            // Find end block
            int BlocksToEnd = 1;
            int ClonedIndex = TokenIndex;
            while (Lexar->getToken(ClonedIndex).tokenType != TokenTypes::NullToken && BlocksToEnd){
                if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::BlockStart){
                    BlocksToEnd++;
                }
                else if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::BlockEnd){
                    BlocksToEnd--;
                }
                ClonedIndex++;
            }
            if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::NullToken){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            Bloc->TokenSize = ClonedIndex - TokenIndex;

            InterpretBlock(File, Function, Bloc, TokenIndex);

            ParentBlock->Block.push_back(SemStatement);
            TokenIndex++;

            return 0;
        }
        else if (Lexar->getToken(TokenIndex).tokenValue == "else" && Lexar->getToken(TokenIndex+1).tokenValue == "if"){
            TokenIndex+=2;

            if(Lexar->getToken(TokenIndex).tokenType != TokenTypes::ArgumentStart){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected '(' but got " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            TokenIndex++;

            SemanticStatment* SemStatement = new SemanticStatment{};
            SemStatement->StateType = SemanticStatment::StatementType::ELSE_IF_STATEMENT;
            SemStatement->Namespace = ParentBlock->Namespace;
            SemStatement->TokenIndex = TokenIndex - 2;

            std::stack<SemanticInstance*> ConditionStack = {};
            std::stack<int> BracketsStack = {};
            BracketsStack.push(0);
            int ArguemntsToEnd = 1;

            SemanticCondition* CurrentOperation = nullptr;
            SemanticOperation* Argument1 = nullptr;

            std::stack<SemanticStatment*> CurrentOperatingStatement = {};
            CurrentOperatingStatement.push(SemStatement);

            // Basically condition bidamas
            while(Lexar->getToken(TokenIndex).tokenType != TokenTypes::NullToken && ArguemntsToEnd > 0){
                if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Comparitor){
                    CurrentOperation = new SemanticCondition();
                    CurrentOperation->Condition = ConditionTypeMap[Lexar->getToken(TokenIndex).tokenValue];
                    CurrentOperation->Namespace = ParentBlock->Namespace;
                    CurrentOperation->TokenIndex = TokenIndex - 2;
                    CurrentOperation->Operation1 = Argument1;
                    Argument1 = nullptr;
                    TokenIndex++;
                    continue;
                }
                else if(Lexar->getToken(TokenIndex).tokenType == BooleanOperator){
                    // Combinations of Conditions
                    if(CurrentOperation || Argument1){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected Boolean Operator within condition " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                        return -1;
                    }

                    SemanticBooleanOperator* BoolOp = new SemanticBooleanOperator();
                    BoolOp->Condition = BooleanOperationTypeMap[Lexar->getToken(TokenIndex).tokenValue];
                    TokenIndex++;
                    CurrentOperatingStatement.top()->ParameterConditions.push_back(BoolOp);

                    continue;
                }
                else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::ArgumentStart){
                    // Only if it contains a comparison otherwise its a operation
                    int ClonedTokenIndex = TokenIndex;
                    bool Process = false;
                    int BlocksToEscape = 0;
                    while(BlocksToEscape >= 0 && Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::NullToken){
                        if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::Comparitor){
                            Process = true;
                            break;
                        }
                        if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::BooleanOperator){
                            Process = true;
                            break;
                        }
                        else if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentStart){
                            BlocksToEscape++;
                        }
                        else if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentEnd){
                            BlocksToEscape--;
                        }
                        ClonedTokenIndex++;
                    }
                    if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::NullToken){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                        return -1;
                    }

                    if(Process){
                        // BracketsStack.push(ConditionStack.size());
                        // Start building a new condition
                        auto newStatement = new SemanticStatment();
                        newStatement->StateType = SemanticStatment::StatementType::BOOLEAN_STATEMENT;
                        newStatement->Namespace = ParentBlock->Namespace;
                        newStatement->TokenIndex = TokenIndex;
                        CurrentOperatingStatement.push(newStatement);

                        ArguemntsToEnd++;
                        TokenIndex++;
                        continue;
                    }
                }
                else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::ArgumentEnd){
                    ArguemntsToEnd--;

                    auto Top = CurrentOperatingStatement.top();
                    CurrentOperatingStatement.pop();

                    if(Top != SemStatement){
                        CurrentOperatingStatement.top()->ParameterConditions.push_back(Top);
                    }

                    // int ToPop = ConditionStack.size() - BracketsStack.top();
                    // BracketsStack.pop();

                    // for(; ToPop > 0; ToPop--){
                    //     CurrentOperatingStatement->ParameterConditions.push_back(ConditionStack.top());
                    //     ConditionStack.pop();
                    // }

                    TokenIndex++;
                    continue;
                }

                // Otherwise interpret as a operation
                // Find end of operation
                int ClonedTokenIndex = TokenIndex;
                int EndToken = TokenIndex - 1;
                int BlocksToEscape = 0;
                while(Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::Comparitor && Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::BooleanOperator && (Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::ArgumentEnd || BlocksToEscape) && Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::NullToken){
                    ClonedTokenIndex++;
                    EndToken++;
                    if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentStart){
                        BlocksToEscape++;
                    }
                    else if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentEnd && BlocksToEscape){
                        BlocksToEscape--;
                    }
                }
                if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::NullToken){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(ClonedTokenIndex).tokenValue << std::endl;
                    return -1;
                }

                // Interpret
                if(!CurrentOperation){
                    InterpretOpration(File, Function, ParentBlock, Argument1, TokenIndex, false, EndToken);
                }
                else{
                    InterpretOpration(File, Function, ParentBlock, CurrentOperation->Operation2, TokenIndex, false, EndToken);
                    CurrentOperatingStatement.top()->ParameterConditions.push_back(CurrentOperation);
                    CurrentOperation = nullptr;
                }

            }

            if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::NullToken){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }

            if(Lexar->getToken(TokenIndex).tokenType != TokenTypes::BlockStart){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected '{' Got " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            TokenIndex++;
            SemanticBlock* Bloc = new SemanticBlock();
            SemStatement->Block = Bloc;
            Bloc->TokenIndex = TokenIndex;
            Bloc->Namespace = SemStatement->Namespace + "__ELSE_IF____" + GenerateID() + "__";
            Bloc->Parent = ParentBlock;
            
            // Find end block
            int BlocksToEnd = 1;
            int ClonedIndex = TokenIndex;
            while (Lexar->getToken(ClonedIndex).tokenType != TokenTypes::NullToken && BlocksToEnd){
                if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::BlockStart){
                    BlocksToEnd++;
                }
                else if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::BlockEnd){
                    BlocksToEnd--;
                }
                ClonedIndex++;
            }
            if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::NullToken){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            Bloc->TokenSize = ClonedIndex - TokenIndex;

            InterpretBlock(File, Function, Bloc, TokenIndex);

            ParentBlock->Block.push_back(SemStatement);
            TokenIndex++;

            return 0;
        }
        else if (Lexar->getToken(TokenIndex).tokenValue == "else"){
            TokenIndex++;

            if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::ArgumentStart){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }

            SemanticStatment* SemStatement = new SemanticStatment{};
            SemStatement->StateType = SemanticStatment::StatementType::ELSE_STATEMENT;
            SemStatement->Namespace = ParentBlock->Namespace;
            SemStatement->TokenIndex = TokenIndex - 2;

            if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::NullToken){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }

            if(Lexar->getToken(TokenIndex).tokenType != TokenTypes::BlockStart){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected '{' Got " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            TokenIndex++;
            SemanticBlock* Bloc = new SemanticBlock();
            SemStatement->Block = Bloc;
            Bloc->TokenIndex = TokenIndex;
            Bloc->Namespace = SemStatement->Namespace + "__ELSE____" + GenerateID() + "__";
            Bloc->Parent = ParentBlock;
            
            // Find end block
            int BlocksToEnd = 1;
            int ClonedIndex = TokenIndex;
            while (Lexar->getToken(ClonedIndex).tokenType != TokenTypes::NullToken && BlocksToEnd){
                if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::BlockStart){
                    BlocksToEnd++;
                }
                else if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::BlockEnd){
                    BlocksToEnd--;
                }
                ClonedIndex++;
            }
            if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::NullToken){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            Bloc->TokenSize = ClonedIndex - TokenIndex;

            InterpretBlock(File, Function, Bloc, TokenIndex);

            ParentBlock->Block.push_back(SemStatement);
            TokenIndex++;

            return 0;
        }
        else if (Lexar->getToken(TokenIndex).tokenValue == "while"){
            TokenIndex++;

            if(Lexar->getToken(TokenIndex).tokenType != TokenTypes::ArgumentStart){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected '(' but got " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            TokenIndex++;

            SemanticStatment* SemStatement = new SemanticStatment{};
            SemStatement->StateType = SemanticStatment::StatementType::WHILE_STATEMENT;
            SemStatement->Namespace = ParentBlock->Namespace;
            SemStatement->TokenIndex = TokenIndex - 2;
            
            int ArguemntsToEnd = 1;

            SemanticCondition* CurrentOperation = nullptr;
            SemanticOperation* Argument1 = nullptr;

            std::stack<SemanticStatment*> CurrentOperatingStatement = {};
            CurrentOperatingStatement.push(SemStatement);

            // Basically condition bidamas
            while(Lexar->getToken(TokenIndex).tokenType != TokenTypes::NullToken && ArguemntsToEnd > 0){
                if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Comparitor){
                    CurrentOperation = new SemanticCondition();
                    CurrentOperation->Condition = ConditionTypeMap[Lexar->getToken(TokenIndex).tokenValue];
                    CurrentOperation->Namespace = ParentBlock->Namespace;
                    CurrentOperation->TokenIndex = TokenIndex - 2;
                    CurrentOperation->Operation1 = Argument1;
                    Argument1 = nullptr;
                    TokenIndex++;
                    continue;
                }
                else if(Lexar->getToken(TokenIndex).tokenType == BooleanOperator){
                    // Combinations of Conditions
                    if(CurrentOperation || Argument1){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected Boolean Operator within condition " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                        return -1;
                    }

                    SemanticBooleanOperator* BoolOp = new SemanticBooleanOperator();
                    BoolOp->Condition = BooleanOperationTypeMap[Lexar->getToken(TokenIndex).tokenValue];
                    TokenIndex++;
                    CurrentOperatingStatement.top()->ParameterConditions.push_back(BoolOp);

                    continue;
                }
                else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::ArgumentStart){
                    // Only if it contains a comparison otherwise its a operation
                    int ClonedTokenIndex = TokenIndex;
                    bool Process = false;
                    int BlocksToEscape = 0;
                    while(BlocksToEscape >= 0 && Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::NullToken){
                        if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::Comparitor){
                            Process = true;
                            break;
                        }
                        if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::BooleanOperator){
                            Process = true;
                            break;
                        }
                        else if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentStart){
                            BlocksToEscape++;
                        }
                        else if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentEnd){
                            BlocksToEscape--;
                        }
                        ClonedTokenIndex++;
                    }
                    if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::NullToken){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                        return -1;
                    }

                    if(Process){
                        // BracketsStack.push(ConditionStack.size());
                        // Start building a new condition
                        auto newStatement = new SemanticStatment();
                        newStatement->StateType = SemanticStatment::StatementType::BOOLEAN_STATEMENT;
                        newStatement->Namespace = ParentBlock->Namespace + "____" + GenerateID() + "__";
                        newStatement->TokenIndex = TokenIndex;
                        CurrentOperatingStatement.push(newStatement);

                        ArguemntsToEnd++;
                        TokenIndex++;
                        continue;
                    }
                }
                else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::ArgumentEnd){
                    ArguemntsToEnd--;

                    auto Top = CurrentOperatingStatement.top();
                    CurrentOperatingStatement.pop();

                    if(Top != SemStatement){
                        CurrentOperatingStatement.top()->ParameterConditions.push_back(Top);
                    }

                    TokenIndex++;
                    continue;
                }

                // Otherwise interpret as a operation
                // Find end of operation
                int ClonedTokenIndex = TokenIndex;
                int EndToken = TokenIndex - 1;
                int BlocksToEscape = 0;
                while(Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::Comparitor && Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::BooleanOperator && (Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::ArgumentEnd || BlocksToEscape) && Lexar->getToken(ClonedTokenIndex).tokenType != TokenTypes::NullToken){
                    ClonedTokenIndex++;
                    EndToken++;
                    if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentStart){
                        BlocksToEscape++;
                    }
                    else if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::ArgumentEnd && BlocksToEscape){
                        BlocksToEscape--;
                    }
                }
                if(Lexar->getToken(ClonedTokenIndex).tokenType == TokenTypes::NullToken){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(ClonedTokenIndex).tokenValue << std::endl;
                    return -1;
                }

                // Interpret
                if(!CurrentOperation){
                    InterpretOpration(File, Function, ParentBlock, Argument1, TokenIndex, false, EndToken);
                }
                else{
                    InterpretOpration(File, Function, ParentBlock, CurrentOperation->Operation2, TokenIndex, false, EndToken);
                    CurrentOperatingStatement.top()->ParameterConditions.push_back(CurrentOperation);
                    CurrentOperation = nullptr;
                }

            }

            if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::NullToken){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }

            if(Lexar->getToken(TokenIndex).tokenType != TokenTypes::BlockStart){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Expected '{' Got " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            TokenIndex++;
            SemanticBlock* Bloc = new SemanticBlock();
            SemStatement->Block = Bloc;
            Bloc->TokenIndex = TokenIndex;
            Bloc->Namespace = SemStatement->Namespace + "__WHILE____" + GenerateID() + "__";
            Bloc->Parent = ParentBlock;
            
            // Find end block
            int BlocksToEnd = 1;
            int ClonedIndex = TokenIndex;
            while (Lexar->getToken(ClonedIndex).tokenType != TokenTypes::NullToken && BlocksToEnd){
                if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::BlockStart){
                    BlocksToEnd++;
                }
                else if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::BlockEnd){
                    BlocksToEnd--;
                }
                ClonedIndex++;
            }
            if(Lexar->getToken(ClonedIndex).tokenType == TokenTypes::NullToken){
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected EOF " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
            Bloc->TokenSize = ClonedIndex - TokenIndex;

            InterpretBlock(File, Function, Bloc, TokenIndex);

            ParentBlock->Block.push_back(SemStatement);
            TokenIndex++;

            return 0;
        }
        else{
            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Statement not interpretable " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
            return -1;
        }

        return -1;
    }

    int InterpretBlock(SemanticisedFile* File, SemanticFunctionDeclaration* Function, SemanticBlock* ParentBlock, int& TokenIndex){
        LexicalAnalyser* Lexar = File->Lexar;

        for(;Lexar->getToken(TokenIndex).tokenType != TokenTypes::BlockEnd;){
            // 3 Types available for loading
            // Statement
            // Operation
            // Block

            // Start with statments
            if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Statement){
                // See what statement
                if(InterpretStatement(File, Function, ParentBlock, TokenIndex)){ // Will increment i
                    return -1;
                }
            }
            else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::TypeDef){
                InterpretVariableDefinition(File, Function, ParentBlock, TokenIndex);
            }
            else if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Identifier){
                // TODO CUSTOM TYPEDEFS
                int TokenIdentifierIndex = TokenIndex;

                // Variable operations
                TokenIndex++;
                if(Lexar->getToken(TokenIndex).tokenType == TokenTypes::Operator){
                    if(Lexar->getToken(TokenIndex).tokenValue == "="){
                        // Assignment
                        TokenIndex++;

                        // Get the operation for store
                        SemanticOperation* AssignementOperation = nullptr;
                        InterpretOpration(File, Function, ParentBlock, AssignementOperation, TokenIndex);

                        // Find Variable and its symbol
                        // TODO OTHER FILES
                        // Load Defined Variables
                        std::stack<SemanticBlock*> blockStack;
                        blockStack.push(ParentBlock);

                        SemanticVariable* VariableAssigned = nullptr;

                        while(!blockStack.empty()){
                            SemanticBlock* CurrentBlock = blockStack.top();
                            blockStack.pop();

                            for(auto v : CurrentBlock->Variables){
                                if (v->Identifier == Lexar->getToken(TokenIdentifierIndex).tokenValue){
                                    // Found the soonest option of the variable
                                    AssignementOperation->ResultStore = "[" + v->Namespace + "__" + v->Identifier + "]";
                                    CurrentBlock = nullptr; // End loop early
                                    VariableAssigned = v;
                                    break;
                                }

                            }

                            if(!CurrentBlock){
                                break;
                            }

                            if(CurrentBlock->Parent){ // Backtrack not forward
                                blockStack.push(CurrentBlock->Parent);
                            }
                        }

                        if(!VariableAssigned){
                            // Look through globals
                            for(auto v : File->Variables){
                                if (v->Identifier == Lexar->getToken(TokenIdentifierIndex).tokenValue){
                                    // Found the soonest option of the variable
                                    AssignementOperation->ResultStore = "[" + v->Namespace + "__" + v->Identifier + "]";
                                    VariableAssigned = v;
                                    break;
                                }
                            }
                        }

                        if(!VariableAssigned){
                            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Variable Not Locatable " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                            return -1;
                        }

                        ParentBlock->Block.push_back(AssignementOperation);

                        TokenIndex++;
                    }
                    else{
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected operator " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                        return -1;
                    }
                }
                else{
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Non-Variable " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                    return -1;
                }
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Unexpected token " << Lexar->getToken(TokenIndex).tokenValue << std::endl;
                return -1;
            }
        }

        return 0;
    }

    int InterpretFunctionBlock(SemanticisedFile* File, SemanticFunctionDeclaration* Function){
        LexicalAnalyser* Lexar = File->Lexar;

        // Function block def
        SemanticBlock* FunctionBlock = new SemanticBlock{};
        FunctionBlock->Namespace = Function->Symbol;
        FunctionBlock->TokenIndex = Function->StartToken;
        FunctionBlock->TokenSize = Function->EndToken - Function->StartToken;
        FunctionBlock->Parent = nullptr;

        int TokenIndex = Function->StartToken;

        if(InterpretBlock(File, Function, FunctionBlock, TokenIndex)){
            return -1;
        }

        if(TokenIndex != Function->EndToken){
            std::cerr << File->AssociatedFile << ":" << Lexar->getToken(TokenIndex).fileLine << " << Semantic::" << __LINE__ << " << " << "Incomplete Block Interperation for function " << Function->Identifier << std::endl;
            return -1;
        }

        Function->Block = FunctionBlock;

        return 0;
    }

}

SemanticAnalyser::SemanticAnalyser(std::vector<LexicalAnalyser*> AnalysisData)
{
    this->AnalysisData = AnalysisData;

    // Store of currently interpreted
    std::vector<LexicalAnalyser*> Analysed = {};

    // Holding of semantic Files
    std::vector<SemanticisedFile*> Interpretations = {};

    // We still need to analyse some
    auto it = AnalysisData.begin();
    while((it = std::find_if(it, AnalysisData.end(), [&Analysed](LexicalAnalyser* item) {
                return std::find(Analysed.begin(), Analysed.end(), item) == Analysed.end();
            })) != AnalysisData.end()) {

        // Treat each file separately
        LexicalAnalyser* Lexar = *it;

        // Create a File
        SemanticisedFile* File = new SemanticisedFile{};

        // Load associated Lexar-File
        File->AssociatedFile = Lexar->GetFilename();
        File->Lexar = Lexar;
        File->ID = GenerateID();

        for(int i = 0; i < Lexar->Tokens().size();){ // Manual Incremenation
            // ONLY ALLOW AT BEGINING
            if(Lexar->getToken(i).tokenType != TokenTypes::Macro){
                break; // Stop looking as we have passed min region
            }
            i++; // "#"

            // See MacroType
            if(Lexar->getToken(i).tokenType != TokenTypes::MacroStatement){
                std::cerr <<  File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Non-Macro Type encountered on line " << std::endl;
                return;
            }
            if(Lexar->getToken(i).tokenValue == "include"){
                i++; // "include"
                // TODO: Search Include Directories (if non-literal)
                if(Lexar->getToken(i).tokenType != TokenTypes::Literal){
                    std::cerr <<  File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Non-Literal include not supported at line " << std::endl;
                    return;
                }
                else{
                    std::string Literal = Lexar->getToken(i).tokenValue;
                    if (Literal[0] != '\"' || Literal[Literal.size() - 1] != '\"'){
                        std::cerr <<  File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Literal include not of type string on line " << std::endl;
                        return;
                    }

                    // Extracted include file (Shoudl be relative to current file)
                    Literal = Literal.substr(1, Literal.size() - 2);

                    // Combine to create new path of include
                    std::string RelativeDoc = File->AssociatedFile.substr(0, std::min(File->AssociatedFile.find_last_of('\\'), File->AssociatedFile.find_last_of('/')) + 1) + Literal;

                    // TODO: Ensure that it tokenised before trying to "pre-load" it
                    // TODO: Create a "pre-tokenised" headers data type to be able to pull from if it is not part of AnalysisData

                    // TODO Actually load it
                    File->Associations.push_back(RelativeDoc);
                }
            }
        }

        // Standard TypeDefs will be global so no need to append them

        // Load all typdef (Recursively)
        if(Classifiers::FindTypeDeclarations(File)){
            return;
        }

        // Load functions
        if(Classifiers::FindFunctions(File, 0, -1, "__" + File->ID + "____GLOB__")){
            return;
        }

        // Load Global Variables
        if(Classifiers::FindGlobalVariables(File)){
            return;
        }

        // Interpret functions
        for(auto f : File->FunctionDefs){
            if(f->IsBuiltin || !f->StartToken){
                continue; // Not definable
            }

            if(Classifiers::InterpretFunctionBlock(File, f)){
                return;
            }
        }

        Analysed.push_back(Lexar);
        Interpretations.push_back(File);
    }

    // Should be fully successfull
    this->LoadedFiles = Interpretations;
}