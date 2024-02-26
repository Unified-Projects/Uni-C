#include <Parsing/SemanticAnalyser.h>

using namespace Parsing;
using namespace Parsing::SemanticVariables;

#include <stack> // RPN

#include <iostream> // Used for error logging

std::map<LexicalAnalyser*, Scope*> SemanticLexerScopeMap = {};

std::map<std::string, SemanticTypeDefinition> SemanticTypeMap = {
    // TypeName : DefinedScope
    std::pair<std::string, SemanticTypeDefinition>("int", {"int", 0, 4, {}, nullptr, 0, 0, 0}),
    std::pair<std::string, SemanticTypeDefinition>("uint", {"uint", 0, 4, {}, nullptr, 0, 0, 1}),
    std::pair<std::string, SemanticTypeDefinition>("short", {"short", 0, 2, {}, nullptr, 0, 0, 2}),
    std::pair<std::string, SemanticTypeDefinition>("ushort", {"ushort", 0, 2, {}, nullptr, 0, 0, 3}),
    std::pair<std::string, SemanticTypeDefinition>("long", {"long", 0, 8, {}, nullptr, 0, 0, 4}),
    std::pair<std::string, SemanticTypeDefinition>("ulong", {"ulong", 0, 8, {}, nullptr, 0, 0, 5}),
    std::pair<std::string, SemanticTypeDefinition>("float", {"float", 0, 4, {}, nullptr, 0, 0, 6}),
    std::pair<std::string, SemanticTypeDefinition>("double", {"double", 0, 8, {}, nullptr, 0, 0, 7}),
    std::pair<std::string, SemanticTypeDefinition>("char", {"char", 0, 1, {}, nullptr, 0, 0, 8}),
    std::pair<std::string, SemanticTypeDefinition>("uchar", {"uchar", 0, 1, {}, nullptr, 0, 0, 9}),
    std::pair<std::string, SemanticTypeDefinition>("bool", {"bool", 0, 1, {}, nullptr, 0, 0, 10}),
    std::pair<std::string, SemanticTypeDefinition>("void", {"void", 0, 1, {}, nullptr, 0, 0, 11}),
    std::pair<std::string, SemanticTypeDefinition>("string", {"string", 0, 1, {}, nullptr, 0, 0, 12}),
    std::pair<std::string, SemanticTypeDefinition>("*", {"*", 0, 8, {}, nullptr, 0, 0, 13}), // TODO Pointers
};

std::map<std::string, SemanticOperationTypes> SemanticOperationMap = {
    {"=", SemanticOperationAssignment},
    {"+", SemanticOperationAddition},
    {"-", SemanticOperationSubtraction},
    {"*", SemanticOperationMultiplication},
    {"/", SemanticOperationDivision},
    {"%", SemanticOperationModulus},
    {"++", SemanticOperationIncrement},
    {"--", SemanticOperationDecrement},
    {"==", SemanticOperationEqual},
    {"!=", SemanticOperationNotEqual},
    {">", SemanticOperationGreater},
    {"<", SemanticOperationLess},
    {">=", SemanticOperationGreaterEqual},
    {"<=", SemanticOperationLessEqual},
    {"&&", SemanticOperationAnd},
    {"||", SemanticOperationOr},
    {"!", SemanticOperationNot},
    {"&", SemanticOperationBitwiseAnd},
    {"|", SemanticOperationBitwiseOr},
    {"~", SemanticOperationBitwiseNot},
    {"^", SemanticOperationBitwiseXor},
    {"<<", SemanticOperationBitwiseLeftShift},
    {">>", SemanticOperationBitwiseRightShift},
    {"?", SemanticOperationTernary},
    {"()", SemanticOperationFunctionCall},
    {"[]", SemanticOperationArrayAccess},
    {".", SemanticOperationMemberAccess},
    {">", SemanticOperationPointerAccess}
};

int SemanticNextTypeID = 13;

std::map<std::string, SemanticFunctionDeclaration> SemanticFunctionMap = {
    // FunctionName : DefinedScope
};

std::map<int, SemanticVariable*> SemanticScopeMap = {
    // ScopePosition : Scope
};

int EvaluateOperands(Operation* OpParent, LexicalAnalyser* Lexer, int* tokenStart){
    int i = *tokenStart;

    // Find Distance to line end
    int LineEnd = -1;
    for(int j = i; j < Lexer->Tokens().size(); j++){
        if(Lexer->getToken(j).tokenType == TokenTypes::LineEnd){
            LineEnd = j;
            break;
        }
    }

    if(LineEnd == -1){
        std::cerr << "Error: Expected ';' at line " << Lexer->getToken(i).fileLine << std::endl;
        return -1;
    }
    else if(LineEnd == i){
        std::cerr << "Error: Expected Operand at line " << Lexer->getToken(i).fileLine << std::endl;
        return -1;
    }

    std::stack<Operation*> OperatorStack = {};

    while(i < LineEnd){
        Token Operand = Lexer->getToken(i);
        switch (Operand.tokenType)
        {
        case TokenTypes::Literal:
            {
                // Load literal
                SemLiteral* lit = new SemLiteral();
                lit->Value = Operand.tokenValue;
                // TODO Custom Intialisers
                if(lit->Value.find_first_not_of("0123456789-") == std::string::npos && ('-' == lit->Value.front() || lit->Value.find('-') == std::string::npos) && std::count(lit->Value.begin(), lit->Value.end(), '-') <= 1){
                    lit->TypeID = SemanticTypeMap["int"].TypeID;
                }
                else if(lit->Value.find_first_not_of("0123456789-.") == std::string::npos && ('-' == lit->Value.front() || lit->Value.find('-') == std::string::npos) && std::count(lit->Value.begin(), lit->Value.end(), '-') <= 1 && std::count(lit->Value.begin(), lit->Value.end(), '.') <= 1){
                    lit->TypeID = SemanticTypeMap["double"].TypeID;
                }
                else if(lit->Value == std::string("true") || lit->Value == std::string("false")){
                    lit->TypeID = SemanticTypeMap["bool"].TypeID;
                }
                else if(lit->Value[0] == '\"' && lit->Value.ends_with("\"")){
                    lit->TypeID = SemanticTypeMap["string"].TypeID;
                }
                else if(lit->Value[0] == '\'' && lit->Value.ends_with("'")){
                    lit->TypeID = SemanticTypeMap["char"].TypeID;
                }
                else{
                    std::cerr << "Error: Expected Literal at line " << Lexer->getToken(i).fileLine << std::endl;
                    return -1;
                }
                lit->LocalScope = OpParent->LocalScope;
                lit->ScopePosition = OpParent->Parent->NextScopePosition++;
                lit->Parent = OpParent;
            
                OpParent->Parameters.push_back(lit);

                i++;
            }
            break;
        case TokenTypes::Identifier:
            {
                // TODO FUNCTIONS

                // Load variable reference
                VariableRef* id = new VariableRef();
                id->Identifier = Operand.tokenValue;
                id->ParentScope = OpParent->LocalScope;
                id->LocalScope = OpParent->LocalScope;
                id->ScopePosition = OpParent->Parent->NextScopePosition++;
                id->Parent = OpParent;

                OpParent->Parameters.push_back(id);

                i++;
            }
            break;
        case TokenTypes::Operator:
            {
                // Check its valid for internal Operations!
                if(SemanticOperationMap.find(Operand.tokenValue) == SemanticOperationMap.end()){
                    std::cerr << "Error: Operator not supported '" << Operand.tokenValue << "' at line " << Lexer->getToken(i).fileLine << std::endl;
                    return -1;
                }

                // Load Operator
                Operation* op = new Operation();
                op->TypeID = SemanticOperationMap[Operand.tokenValue];
                op->ParentScope = OpParent->LocalScope;
                op->LocalScope = OpParent->LocalScope;
                op->ScopePosition = OpParent->Parent->NextScopePosition++;
                op->Parent = OpParent;

                i++;

                while( OperatorStack.size() > 0 && OperatorStack.top()->TypeID >= op->TypeID){
                    OpParent->Parameters.push_back(OperatorStack.top());
                    OperatorStack.pop();
                }
                OperatorStack.push(op);
            }
            break;
        case TokenTypes::ArgumentStart:
            {
                Operation* op = new Operation();
                op->TypeID = SemanticOperationScoper;
                op->ParentScope = OpParent->LocalScope;
                op->LocalScope = OpParent->LocalScope;
                op->Parent = OpParent;

                OperatorStack.push(op);
                i++;
            }
            break;
        case TokenTypes::ArgumentEnd:
            {
                while(OperatorStack.size() > 0 && OperatorStack.top()->TypeID != SemanticOperationScoper){
                    OpParent->Parameters.push_back(OperatorStack.top());
                    OperatorStack.pop();
                }
                OperatorStack.pop();
                i++;
            }
            break;
        default:
            std::cerr << "Error: Operand Type Not Supported at line " << Lexer->getToken(i).fileLine << std::endl; // TODO
            return -1;
            break;
        }   
    }
 
    while(OperatorStack.size() > 0){
        OpParent->Parameters.push_back(OperatorStack.top());
        OperatorStack.pop();
    }

    tokenStart[0] = i;

    return 0;
}

SemanticVariable* Evaluate(SemanticVariable* Parent, std::vector<SemanticVariable*>* Store,  LexicalAnalyser* Lexer, int* tokenStart){
    int i = *tokenStart;
    Token token = Lexer->getToken(i);

    SemanticVariable* Result = nullptr;

    switch (token.tokenType)
    {
    case TokenTypes::TypeDef:
        {   
            // Start building variable
            Variable* var = new Variable();
            var->ParentScope = Parent->LocalScope;
            var->LocalScope = Parent->LocalScope;
            var->ScopePosition = Parent->NextScopePosition++;
            var->Parent = Parent;

            // Get Identifier
            if(Lexer->getToken(i+1).tokenType == TokenTypes::Identifier){
                var->Identifier = Lexer->getToken(i+1).tokenValue;
            }
            else{
                std::cerr << "Error: Expected Identifier at line " << Lexer->getToken(i+1).fileLine << std::endl;
                return nullptr;
            }

            // Get Type
            var->TypeID = SemanticTypeMap[Lexer->getToken(i).tokenValue].TypeID;

            // Initial Value If Present
            if(Lexer->getToken(i+2).tokenType != TokenTypes::LineEnd){ // Unless Value!! // TODO
                if(Lexer->getToken(i+2).tokenType == TokenTypes::Operator && Lexer->getToken(i+2).tokenValue == std::string("=")){
                    if(Lexer->getToken(i+3).tokenType == TokenTypes::Literal){
                        var->InitValue = Lexer->getToken(i+3).tokenValue;
                        if(Lexer->getToken(i+4).tokenType != TokenTypes::LineEnd){
                            std::cerr << "Error: Expected ';' at line " << Lexer->getToken(i+4).fileLine << std::endl;
                            return nullptr;
                        }
                    }
                    else{
                        std::cerr << "Error: Expected Literal at line " << Lexer->getToken(i+3).fileLine << std::endl;
                        return nullptr;
                    }
                }
                else{
                    std::cerr << "Error: Expected ';' at line " << Lexer->getToken(i+2).fileLine << std::endl;
                    return nullptr;
                }

                i+=2; // Skip forwards
            }

            // Token Index Correction
            i += 2;

            Store->push_back(var);
            Result = var;
        }
        break;

    case TokenTypes::Identifier:
        {
            // Variable Initialisation
            // TODO Variable Initialisation of custom typeDef

            // Operation
            // Store Variable
            std::string VariableName = token.tokenValue;

            // Check for Operation
            if(Lexer->getToken(i+1).tokenType == TokenTypes::Operator){
                if(Lexer->getToken(i+1).tokenValue == std::string("=")){ // Assignment
                    // Create Operator
                    Operation* op = new Operation();
                    op->TypeID = SemanticOperationAssignment;
                    op->ParentScope = Parent->LocalScope;
                    op->LocalScope = Parent->LocalScope;
                    op->ScopePosition = Parent->NextScopePosition++;
                    op->Parent = Parent;

                    // Get Operand
                    if(EvaluateOperands(op, Lexer, &i)){
                        std::cerr << "Operator Invalid at line " << Lexer->getToken(i+1).fileLine << std::endl;
                        return nullptr;
                    }

                    // Current Token is line end
                    if(Lexer->getToken(i).tokenType != TokenTypes::LineEnd){
                        std::cerr << "Error: Expected ';' at line " << Lexer->getToken(i+1).fileLine << std::endl;
                        return nullptr;
                    }

                    Store->push_back(op);
                    Result = op;
                }
                else{
                    std::cerr << "Opeator not supported '" << Lexer->getToken(i+1).tokenValue << "' at line " << Lexer->getToken(i+1).fileLine << std::endl;
                    return nullptr;
                }
            }
        }
        break;

    case TokenTypes::Statement:
        {
            // Statement
            if(token.tokenValue == std::string("return")){
                // Should have a literal then a line end or a identifier then a line end
                if (Lexer->getToken(i+1).tokenType == TokenTypes::Literal || Lexer->getToken(i+1).tokenType == TokenTypes::Identifier){
                    if(Lexer->getToken(i+2).tokenType != TokenTypes::LineEnd){
                        std::cerr << "Error: Expected ';' at line " << Lexer->getToken(i+2).fileLine << std::endl;
                        return nullptr;
                    }

                    // Valid Return
                    SemStatement* ret = new SemStatement();
                    ret->TypeID = SemanticStatementTypes::SemanticStatementReturn;
                    ret->ParentScope = Parent->LocalScope;
                    ret->LocalScope = Parent->LocalScope;
                    ret->ScopePosition = Parent->NextScopePosition++;
                    ret->Parent = Parent;

                    // Load argument
                    if(Lexer->getToken(i+1).tokenType == TokenTypes::Literal){
                        SemLiteral* lit = new SemLiteral();
                        lit->Value = Lexer->getToken(i+1).tokenValue;
                        // TODO Custom Intialisers
                        if(lit->Value.find_first_not_of("0123456789-") == std::string::npos && ('-' == lit->Value.front() || lit->Value.find('-') == std::string::npos) && std::count(lit->Value.begin(), lit->Value.end(), '-') <= 1){
                            lit->TypeID = SemanticTypeMap["int"].TypeID;
                        }
                        else if(lit->Value.find_first_not_of("0123456789-.") == std::string::npos && ('-' == lit->Value.front() || lit->Value.find('-') == std::string::npos) && std::count(lit->Value.begin(), lit->Value.end(), '-') <= 1 && std::count(lit->Value.begin(), lit->Value.end(), '.') <= 1){
                            lit->TypeID = SemanticTypeMap["double"].TypeID;
                        }
                        else if(lit->Value == std::string("true") || lit->Value == std::string("false")){
                            lit->TypeID = SemanticTypeMap["bool"].TypeID;
                        }
                        else if(lit->Value[0] == '\"' && lit->Value.ends_with("\"")){
                            lit->TypeID = SemanticTypeMap["string"].TypeID;
                        }
                        else if(lit->Value[0] == '\'' && lit->Value.ends_with("'")){
                            lit->TypeID = SemanticTypeMap["char"].TypeID;
                        }
                        else{
                            std::cerr << "Error: Expected Literal at line " << Lexer->getToken(i+1).fileLine << std::endl;
                            return nullptr;
                        }

                        lit->ParentScope = ret->LocalScope;
                        lit->LocalScope = ret->LocalScope;
                        lit->ScopePosition = ret->Parent->NextScopePosition++;
                        lit->Parent = ret;

                        ret->Parameters.push_back(lit);

                        i+=3; // Skip forwards
                    }
                    else if(Lexer->getToken(i+1).tokenType == TokenTypes::Identifier){
                        VariableRef* id = new VariableRef();
                        id->Identifier = Lexer->getToken(i+1).tokenValue;
                        id->ParentScope = ret->LocalScope;
                        id->LocalScope = ret->LocalScope;
                        id->ScopePosition = ret->Parent->NextScopePosition++;
                        id->Parent = ret;

                        ret->Parameters.push_back(id);

                        i+=3; // Skip forwards
                    }
                    else{
                        std::cerr << "Error: Expected Literal or Identifier at line " << Lexer->getToken(i+1).fileLine << std::endl;
                        return nullptr;
                    }

                    Store->push_back(ret);
                    Result = ret;
                }
                else{
                    std::cerr << "Error: Expected Literal or Identifier at line " << Lexer->getToken(i+1).fileLine << std::endl;
                    return nullptr;
                }
            }
        }
        break;
    
    default:
        break;
    }

    tokenStart[0] = i;

    return Result;
}

SemanticAnalyser::SemanticAnalyser(std::vector<LexicalAnalyser*> AnalysisData)
{
    this->AnalysisData = AnalysisData;

    this->RootScope = nullptr; // If remains null, error encountered

    // Create root scope
    Scope* rootScope = new Scope();
    rootScope->LocalScope = 0;
    rootScope->ParentScope = -1;
    rootScope->ScopePosition = 0;
    rootScope->Parent = nullptr;
    rootScope->Block = {};
    
    SemanticScopeMap[0] = rootScope;

    int NextScope = 1; 

    { // Create Lexer Scopemap
        for(auto& Lexer : AnalysisData){
            // Create new scope for data
            Scope* LexerScope = new Scope();
            LexerScope->LocalScope = NextScope++;
            LexerScope->ParentScope = rootScope->LocalScope;
            LexerScope->ScopePosition = rootScope->NextScopePosition++;
            LexerScope->Parent = rootScope;
            LexerScope->Block = {};

            // Add to scopeMap
            SemanticLexerScopeMap[Lexer] = LexerScope;

            // Add to general Scope Map
            SemanticScopeMap[LexerScope->LocalScope] = LexerScope;

            rootScope->Block.push_back(LexerScope);
        }
    }

    { // Create TypeMap
        // Traverse through AnalysisData
        for(auto& Lexer : AnalysisData){
            // Load Scope
            Scope* LexerScope = SemanticLexerScopeMap[Lexer];

            // Traverse through tokens
            int TokenCount = Lexer->Tokens().size();
            for(int i = 0; i < TokenCount; i++){
                // Get Current Token
                Token token = Lexer->getToken(i);

                // Check for context Statment
                if(token.tokenType = TokenTypes::Statement){
                    // Could be correct Statement
                    if(token.tokenValue == std::string("struct") || token.tokenValue == std::string("class")){ // TODO using x = y
                        // Check for type definition
                        if(Lexer->getToken(i+1).tokenType == TokenTypes::Identifier){
                            // Check for block start
                            if(Lexer->getToken(i+2).tokenType == TokenTypes::BlockStart){ // TODO Inheritance Check
                                // Check for block end
                                int blockStart = i+2;
                                int blockEnd = -1;
                                int blockDepth = 0;
                                for(int j = i+3; j < Lexer->Tokens().size(); j++){
                                    if(Lexer->getToken(j).tokenType == TokenTypes::BlockEnd){
                                        blockEnd = j;
                                        break;
                                    }
                                    else{
                                        blockDepth++;
                                    }
                                }

                                // Non-Ended
                                if(blockEnd == -1){
                                    std::cerr << "Error: " << Lexer->getToken(i+1).tokenValue << " Missing Ending '}'" << std::endl;
                                    return;
                                }
                                if(Lexer->getToken(blockEnd+1).tokenType != TokenTypes::LineEnd){
                                    std::cerr << "Error: " << Lexer->getToken(i+1).tokenValue << " Missing Ending ';' at line " << Lexer->getToken((blockEnd+1)).fileLine << std::endl;
                                    return;
                                }

                                if(blockEnd != -1){
                                    // Reat block attributes
                                    std::vector<SemanticTypeAttribute> Attributes = {};
                                    int totalSize = 0;

                                    // Find attributes in block
                                    for(int j = blockStart + 1; j < blockEnd; j+=3){
                                        if(Lexer->getToken(j).tokenType == TokenTypes::TypeDef || (Lexer->getToken(j).tokenType == TokenTypes::Identifier && SemanticTypeMap[Lexer->getToken(j).tokenValue].Size > 0)){
                                            // Type
                                            std::string Type = Lexer->getToken(j).tokenValue;

                                            // Identifier
                                            std::string Identifier = Lexer->getToken(j+1).tokenValue;

                                            for(auto &p : Attributes){
                                                if(p.Name == Identifier){
                                                    std::cerr << "Error: " << Lexer->getToken(i+1).tokenValue << " has Duplicate Identifier " << Identifier << " at line " << Lexer->getToken(j+1).fileLine << std::endl;
                                                    return;
                                                }
                                            }

                                            // Add size
                                            totalSize += SemanticTypeMap[Type].Size;

                                            std::string InitVal = "";

                                            if(Lexer->getToken(j+2).tokenType != TokenTypes::LineEnd){
                                                if(Lexer->getToken(j+2).tokenType == TokenTypes::Operator && Lexer->getToken(j+2).tokenValue == std::string("=")){
                                                    if(Lexer->getToken(j+2).tokenType == TokenTypes::Operator && Lexer->getToken(j+2).tokenValue == std::string("=")){
                                                        // Check for Literal
                                                        if(Lexer->getToken(j+3).tokenType == TokenTypes::Literal){
                                                            InitVal = Lexer->getToken(j+3).tokenValue;

                                                            if(Lexer->getToken(j+4).tokenType != TokenTypes::LineEnd){
                                                                std::cerr << "Error: Expected ';' at line " << Lexer->getToken(j+4).fileLine << std::endl;
                                                                return;
                                                            }

                                                            // Offset Correction
                                                            j+=2;
                                                        }
                                                        else{
                                                            // Custom Initialiser TBD // TODO
                                                            std::cerr << "Error: Expected Literal at line " << Lexer->getToken(j+3).fileLine << std::endl;
                                                        }
                                                    }
                                                }
                                                else{
                                                    std::cerr << "Error: Expected ';' at line " << Lexer->getToken(j+2).fileLine << std::endl;
                                                    return;
                                                }
                                            }

                                            // Load to Attribute Map
                                            Attributes.push_back({Type, Identifier, InitVal});
                                        }
                                        else{
                                            std::cerr << "Error: Expected Type at line " << Lexer->getToken(j).fileLine << std::endl;
                                            return;
                                        }
                                    }

                                    // Create TypeDefinition
                                    SemanticTypeDefinition typeDef = {
                                        Lexer->getToken(i+1).tokenValue,
                                        LexerScope->LocalScope,
                                        totalSize,
                                        Attributes,
                                        Lexer,
                                        blockStart,
                                        blockEnd,
                                        SemanticNextTypeID++,
                                        i,
                                        blockEnd
                                    };

                                    // Add to TypeMap
                                    SemanticTypeMap[Lexer->getToken(i+1).tokenValue] = typeDef;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    { // Create FunctionMap
        // Traverse through AnalysisData
        for(auto& Lexer : AnalysisData){
            // Load Scope
            Scope* LexerScope = SemanticLexerScopeMap[Lexer];

            // Traverse through tokens
            int TokenCount = Lexer->Tokens().size();
            for(int i = 0; i < TokenCount; i++){
                // Get Current Token
                Token token = Lexer->getToken(i);

                // First check for function declaration
                /*
                    Type Identifier(Parameters){}
                    Type Identifier(Parameters)=>{} // TODO
                    Identifier(Parameters){}:Type // TODO
                    Identifier(Parameters)=>{}:Type // TODO

                    // TODO Special Case
                    // TODO Custom Types
                    // TODO Operators
                */

                if (i != 0){
                    // See if the previous token was a statement
                    if(Lexer->getToken(i - 1).tokenType == TokenTypes::Statement){
                        continue; // TODO Operator
                    }
                }

                // TODO Achnowledge if in struct and if so change scope

                /*
                    Type Identifier (Parameters) {}
                */ 
                if(token.tokenType == TokenTypes::TypeDef || (token.tokenType == TokenTypes::Identifier && SemanticTypeMap[token.tokenValue].Size > 0)){
                    // Check for Identifier
                    if(Lexer->getToken(i+1).tokenType == TokenTypes::Identifier){
                        // Found name
                        std::string Name = Lexer->getToken(i+1).tokenValue;
                        std::string ReturnType = token.tokenValue;
                        std::vector<SemanticTypeAttribute> Parameters = {}; // Type, Name

                        // Check for Parameters
                        if(Lexer->getToken(i+2).tokenType == TokenTypes::ArgumentStart){
                            int ArgumentEndIndex = i+3; // Index if no args
                            if(Lexer->getToken(i+3).tokenType != TokenTypes::ArgumentEnd){
                                int Offset = 0;
                                // Load parameters
                                while(Lexer->getToken(i+3 + Offset).tokenType == TokenTypes::TypeDef || (Lexer->getToken(i+3 + Offset).tokenType == TokenTypes::Identifier && SemanticTypeMap[Lexer->getToken(i+3 + Offset).tokenValue].Size > 0)){
                                    // Type
                                    std::string Type = Lexer->getToken(i + 3 + Offset).tokenValue;

                                    // Identifier
                                    std::string ID = Lexer->getToken(i + 4 + Offset).tokenValue;

                                    // Default Value
                                    std::string InitVal = "";

                                    if(Lexer->getToken(i + 5 + Offset).tokenType == TokenTypes::ArgumentSeparator){
                                        Offset += 3;
                                    }
                                    else if(Lexer->getToken(i + 5 + Offset).tokenType != TokenTypes::ArgumentEnd){
                                        if(Lexer->getToken(i + 5 + Offset).tokenType == TokenTypes::Operator && Lexer->getToken(i + 5 + Offset).tokenValue == std::string("=")){
                                            if(Lexer->getToken(i + 5 + Offset).tokenType == TokenTypes::Operator && Lexer->getToken(i + 5 + Offset).tokenValue == std::string("=")){
                                                // Check for Literal
                                                if(Lexer->getToken(i + 6 + Offset).tokenType == TokenTypes::Literal){
                                                    InitVal = Lexer->getToken(i + 6 + Offset).tokenValue;

                                                    if(Lexer->getToken(i + 7 + Offset).tokenType != TokenTypes::ArgumentEnd && Lexer->getToken(i + 7 + Offset).tokenType != TokenTypes::ArgumentSeparator){
                                                        std::cerr << "Error: Expected ')' at line " << Lexer->getToken(i + 7 + Offset).fileLine << std::endl;
                                                        return;
                                                    }
                                                }
                                                else{
                                                    // Custom Initialiser TBD // TODO
                                                    std::cerr << "Error: Expected Literal at line " << Lexer->getToken(i + 6 + Offset).fileLine << std::endl;
                                                }
                                            }
                                        }
                                        else{
                                            std::cerr << "Error: Expected ')' at line " << Lexer->getToken(i + 5 + Offset).fileLine << std::endl;
                                            return;
                                        }
                                        Offset += 2;
                                    }

                                    // Load to Parameter Map
                                    Parameters.push_back({Type, ID, InitVal});
                                }
                                if(Lexer->getToken(i+3+Offset).tokenType != TokenTypes::ArgumentEnd){
                                    std::cerr << "Error: Expected ')' or Non-Type parameter present at line " << Lexer->getToken(i+3).fileLine << std::endl;
                                    return;
                                }

                                ArgumentEndIndex = i+3+Offset;
                            }

                            // Load Block
                            if(Lexer->getToken(ArgumentEndIndex+1).tokenType == TokenTypes::BlockStart){
                                // Check for block end
                                int blockStart = ArgumentEndIndex+1;
                                int blockEnd = -1;
                                int blockDepth = 0;
                                for(int j = blockStart + 1; j < Lexer->Tokens().size(); j++){
                                    if(Lexer->getToken(j).tokenType == TokenTypes::BlockEnd){
                                        blockEnd = j;
                                        break;
                                    }
                                    else{
                                        blockDepth++;
                                    }
                                }

                                // Non-Ended
                                if(blockEnd == -1){
                                    std::cerr << "Error: " << Name << " Missing Ending '}'" << std::endl;
                                    return;
                                }

                                if(blockEnd != -1){
                                    // Create FunctionDeclaration
                                    SemanticFunctionDeclaration funcDef = {
                                        Name,
                                        ReturnType,
                                        Parameters,
                                        LexerScope->LocalScope,
                                        Lexer,
                                        blockStart,
                                        blockEnd,
                                        i,
                                        blockEnd
                                    };

                                    // Add to FunctionMap
                                    SemanticFunctionMap[Name] = funcDef;
                                }
                            }
                            else{
                                std::cerr << "Error: Expected '{' at line " << Lexer->getToken(ArgumentEndIndex+1).fileLine<< std::endl;
                                return;
                            }
                        }
                        else{
                            // Could be a typedef so ignore
                        }
                    }
                    else{
                        std::cerr << "Error: Expected Identifier at line " << Lexer->getToken(i+1).fileLine << std::endl;
                        continue;
                    }
                }

            }
        }
    }

    // Load Types to Scopes
    {
        for(auto& Lexer : AnalysisData){
            // Load Scope
            Scope* LexerScope = SemanticLexerScopeMap[Lexer];

            for(auto& p : SemanticTypeMap){
                if(p.second.Scope == LexerScope->LocalScope){
                    // Found scope its a part of

                    // Start building type variable
                    SemTypeDef* type = new SemTypeDef();
                    type->Identifier = p.first;
                    type->Size = p.second.Size;
                    type->TypeID = p.second.TypeID;
                    type->ParentScope = LexerScope->LocalScope; // TODO Check if defined within function
                    type->LocalScope = NextScope++;
                    type->ScopePosition = LexerScope->NextScopePosition++;
                    type->Parent = LexerScope;

                    // Load to scope map
                    SemanticScopeMap[type->LocalScope] = type;

                    // Load functions // TODO

                    // Load attributes
                    for(auto& at : p.second.Attributes){
                        // Start building variable
                        Variable* var = new Variable();
                        var->Identifier = at.Name;
                        var->TypeID = SemanticTypeMap[at.Type].TypeID;
                        var->InitValue = at.Value;
                        // var->Type = SemanticTypes::SemanticTypeVariable;
                        var->ParentScope = type->LocalScope;
                        var->LocalScope = type->LocalScope;
                        var->ScopePosition = type->NextScopePosition++;
                        var->Parent = type;

                        type->Attributes.push_back(var);
                    }

                    // Load to Scope
                    LexerScope->Block.push_back(type);
                }
            }
        }
    }

    // Load Scope Based Global-Variables
    {
        for(auto& Lexer : AnalysisData){
            // Load Scope
            Scope* LexerScope = SemanticLexerScopeMap[Lexer];

            int TokenCount = Lexer->Tokens().size();
            for(int i = 0; i < TokenCount; i++){
                // Check we are not in any structs or functions using typeMap and functionMap to see what blocks are not-accessable
                bool inFunction = false;
                for(auto& fp : SemanticFunctionMap){
                    if(i >= fp.second.BlockStart && i <= fp.second.BlockEnd){
                        inFunction = true;
                        break;
                    }
                }
                if(inFunction) continue;

                bool inStruct = false;
                for(auto& tp : SemanticTypeMap){
                    if(i >= tp.second.BlockStart && i <= tp.second.BlockEnd){
                        inStruct = true;
                        break;
                    }
                }
                if(inStruct) continue;

                if(Lexer->getToken(i).tokenType == TokenTypes::TypeDef || (Lexer->getToken(i).tokenType == TokenTypes::Identifier && SemanticTypeMap[Lexer->getToken(i).tokenValue].Size > 0)){
                    if(Lexer->getToken(i+1).tokenType != TokenTypes::Identifier){
                        std::cerr << "Error: Expected Identifier at line " << Lexer->getToken(i+1).fileLine << std::endl;
                        return;
                    }

                    std::string InitVal = "";

                    if(Lexer->getToken(i+2).tokenType != TokenTypes::LineEnd){ // Unless Value!! // TODO
                        if(Lexer->getToken(i+2).tokenType == TokenTypes::Operator && Lexer->getToken(i+2).tokenValue == std::string("=")){
                            if(Lexer->getToken(i+3).tokenType == TokenTypes::Literal){
                                InitVal = Lexer->getToken(i+3).tokenValue;
                                if(Lexer->getToken(i+4).tokenType != TokenTypes::LineEnd){
                                    std::cerr << "Error: Expected ';' at line " << Lexer->getToken(i+4).fileLine << std::endl;
                                    return;
                                }
                            }
                            else{
                                std::cerr << "Error: Expected Literal at line " << Lexer->getToken(i+3).fileLine << std::endl;
                                return;
                            }
                        }
                        else{
                            std::cerr << "Error: Expected ';' at line " << Lexer->getToken(i+2).fileLine << std::endl;
                            return;
                        }
                    }
                    
                    // Start building variable
                    Variable* var = new Variable();
                    var->Identifier = Lexer->getToken(i+1).tokenValue;
                    var->TypeID = SemanticTypeMap[Lexer->getToken(i).tokenValue].TypeID;
                    var->InitValue = InitVal;
                    // var->Type = SemanticTypes::SemanticTypeVariable;
                    var->ParentScope = LexerScope->LocalScope;
                    var->LocalScope = LexerScope->LocalScope;
                    var->ScopePosition = LexerScope->NextScopePosition++;
                    var->Parent = LexerScope;

                    LexerScope->Block.push_back(var);
                }
            }
        }
    }

    // Load functions
    {
        // TODO Fix embeded functions so that they dont get parsed as global functions
        for(auto& function : SemanticFunctionMap){
            // Load Scope
            Scope* LexerScope = SemanticLexerScopeMap[function.second.Lexer];

            // TODO Functions within functions

            // Start building function variable
            Function* func = new Function();
            func->Identifier = function.first;
            func->ReturnTypeID = SemanticTypeMap[function.second.ReturnType].TypeID;
            func->ParentScope = LexerScope->LocalScope; // TODO : May be type not main scope
            func->LocalScope = NextScope++;
            func->ScopePosition = LexerScope->NextScopePosition++;
            func->Parent = LexerScope;

            // Load to scope map
            SemanticScopeMap[func->LocalScope] = func;

            // Load parameters
            for(auto& p : function.second.Parameters){
                // Start building variable
                Variable* var = new Variable();
                var->Identifier = p.Name;
                var->TypeID = SemanticTypeMap[p.Type].TypeID;
                // var->Type = SemanticTypes::SemanticTypeVariable;
                var->ParentScope = func->LocalScope;
                var->LocalScope = func->LocalScope;
                var->ScopePosition = func->NextScopePosition++;
                var->Parent = func;
                var->InitValue = p.Value;

                func->Parameters.push_back(var);
            }

            // Load block data
            for(int i = function.second.TokenIndexOfBlockStart + 1; i < function.second.TokenIndexOfBlockEnd; i++){
                SemanticVariable* Result = Evaluate(func, &func->Block, function.second.Lexer, &i);
                if(!Result){
                    std::cerr << "Error: Invalid Syntax at line " << function.second.Lexer->getToken(i).fileLine << std::endl;
                    return;
                }
            }

            // Load to scope
            LexerScope->Block.push_back(func);
        }
    }

    // Load root scope
    this->RootScope = rootScope;
}