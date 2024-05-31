#include <Parsing/SemanticAnalyser.h>

using namespace Parsing;
using namespace Parsing::SemanticVariables;

#include <stack> // RPN
#include <algorithm>

#include <iostream> // Used for error logging

std::map<int, std::string> SemanticFunctionArgs{
    // ArgNumber : register
    {0, "rdi"},
    {1, "rsi"},
    {2, "rdx"},
    {3, "rcx"},
    {4, "r8"},
    {5, "r9"},
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
    {"string", new SemanticTypeDefDeclaration{"string", 12, false, {false, ""}, "__GLOB__"}}
};
std::map<SemanticTypeDefDeclaration*, SemanticObjectDefinition*> StandardObjectTypes{
    {StandardisedTypes["string"], new SemanticObjectDefinition{"string", {new SemanticVariable{true, false, false, false, false, "ulong", "Length", "0", "__GLOB____CLASS____string__", -1}, new SemanticVariable{true, false, false, false, true, "char", "Data", "0", "__GLOB____CLASS____string__", -1}}, {/*TODO Things like Size, Substr, FirstOf...*/}, "__GLOB__", "", -1, -1}} // String
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
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'}' expected" << std::endl;
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
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Type Expected" << std::endl;
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
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "Identifier Expected" << std::endl;
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
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'}' expected" << std::endl;
                    return -1;
                }
                continue; // Skip the function declaration
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "';' Expected" << std::endl;
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
                        std::cerr << File->AssociatedFile << ":" << NewIdentifier.fileLine << " << Semantic::" << __LINE__ << " << " << "';' expected" << std::endl;
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
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'{' expected" << std::endl;
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
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'}' expected" << std::endl;
                        return -1;
                    }
                    
                    int TokenEnd = i - 2;

                    if(Lexar->getToken(i).tokenType != TokenTypes::LineEnd){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "';' Expected" << std::endl;
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
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::ArgumentEnd || Lexar->getToken(i).tokenType == TokenTypes::ArgumentSeparator){
                // No Initial Value
                i++;
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "')' or ',' Expected" << std::endl;
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
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "';' Expected" << std::endl;
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
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "';' Expected" << std::endl;
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
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "')' Expected" << std::endl;
                    return -1;
                }
            }

            // Block data
            if(Lexar->getToken(i).tokenType == TokenTypes::LineEnd){
                // See if it is defined
                for(auto F : File->FunctionDefs){
                    if(F->Symbol == NameSpace + "__" + FunctionName + "__"){
                        std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << F->Symbol << "Already defined" << std::endl;
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

                // Load Function Parameters;
                if(FindFunctionParameters(File, FuncDef, StartArgument, ArgumentEnd, NameSpace)){
                    return -1;
                }

                File->FunctionDefs.push_back(FuncDef);

                i++;
                continue;
            }
            else if(Lexar->getToken(i).tokenType == TokenTypes::BlockStart){
                // TODO COULD DEFINE A ASSOCIATED FILE

                // Find end Block
                int TokenStart = i;
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

                if(Lexar->getToken(i).tokenType == TokenTypes::NullToken && Lexar->getToken(i-1).tokenType != TokenTypes::BlockEnd){
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'}' expected" << std::endl;
                    return -1;
                }
                
                int TokenEnd = i - 1;

                FunctionStartToken = TokenStart;
                FunctionEndToken = TokenEnd;
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'{' or ';' Expected" << std::endl;
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
            FuncDef->Symbol = NameSpace + "____FUNC____" + FunctionName + "__";
            FuncDef->Static = Static;
            FuncDef->Private = Private;
            FuncDef->StartToken = FunctionStartToken;
            FuncDef->EndToken = FunctionEndToken;

            if(FuncDef->Parameters.size() == 0){ // TODO Make sure the functions paramenters match when "redefining"
                // Load Function Parameters
                if(FindFunctionParameters(File, FuncDef, StartArgument, ArgumentEnd, NameSpace)){
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
                    std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "'}' expected" << std::endl;
                    return -1;
                }
                continue; // Skip the function declaration
            }
            else{
                std::cerr << File->AssociatedFile << ":" << Lexar->getToken(i).fileLine << " << Semantic::" << __LINE__ << " << " << "';' Expected" << std::endl;
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
        if(Classifiers::FindFunctions(File)){
            return;
        }

        // Load Global Variables
        if(Classifiers::FindGlobalVariables(File)){
            return;
        }

        Analysed.push_back(Lexar);
        Interpretations.push_back(File);
    }

    // Should be fully successfull
    this->LoadedFiles = Interpretations;
}