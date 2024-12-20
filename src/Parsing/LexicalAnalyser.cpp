#include <Parsing/LexicalAnalyser.h>
#include <Parsing/UtilityFunctions.h>

#include <algorithm>

#include <iostream>

using namespace Parsing;

// Statements
std::vector<std::string> ValidStatements = {
    "if",
    "else",
    "while",
    "for",
    // "do",
    // "switch",
    // "case",
    // "default",
    // "break",
    // "continue",
    "return",
    // "goto",
    // "asm",
    // "try",
    // "catch",
    // "throw",
    "using",
    // "enum",
    "struct",
    "class",
    // "namespace",
    // "template",
    "static",
    // "extern",
    // "mutable",
    // "inline",
    // "virtual",
    // "explicit",
    // "friend",
    "public",
    "protected",
    "private",
    "const",
    // "volatile",
    // "alignof",
    // "decltype",
    // "noexcept",
    // "nullptr",
    // "sizeof",
    // "typeid"
};

std::vector<std::string> ValidMacroStatements = {
    "include"
};

std::vector<std::string> StandardTypeDefs = {
    "int",
    "uint",
    "short",
    "ushort",
    "long",
    "ulong",
    "float", // TODO REMOVE!
    "double",
    "char",
    "uchar",
    "bool",
    "void",
    "string"
};

LexicalAnalyser::LexicalAnalyser(const std::string& Data, const std::string FilePath)
    : AssociatedFile(FilePath)
{
    // Iterate over data
    std::string Stream = Data;
    int Line = 1;

    std::string CurrentLine = Stream.substr(0, Stream.find_first_of('\n'));

    while (Data.size() > 0)
    {
        int LineSave = Line;
        Stream = EatWhitespace(Stream, &Line);

        if(Stream.size() == 0){
            break;
        }

        if(Line != LineSave){
            CurrentLine = Stream.substr(0, Stream.find_first_of('\n'));
        }

        LineSave = Line;
        Stream = EatWhitespace(Stream, &Line);

        if(Line != LineSave){
            CurrentLine = Stream.substr(0, Stream.find_first_of('\n'));
        }
 
        if(Stream.size() == 0){
            break;
        }

        // Check for words
        std::string Word = GetWord(Stream);
        std::string WordI = GetWord(Stream, true);
        if(WordI.size() > 0 && WordI.find_first_not_of("0123456789.-") == std::string::npos && ('-' == WordI.front() || WordI.find('-') == std::string::npos) && std::count(WordI.begin(), WordI.end(), '.') <= 1 && std::count(WordI.begin(), WordI.end(), '-') <= 1 && (std::count(WordI.begin(), WordI.end(), '.') + std::count(WordI.begin(), WordI.end(), '-') < WordI.size())){
            tokens.push_back(Token{WordI, TokenTypes::Literal, Line, CurrentLine});
            Stream = EatWord(Stream, true);
        }
        else if(Word.size() > 0){
            bool Cont = false;
            if(std::find(ValidStatements.begin(), ValidStatements.end(), Word) != ValidStatements.end()){
                tokens.push_back(Token{Word, TokenTypes::Statement, Line, CurrentLine});
            }
            else if(std::find(StandardTypeDefs.begin(), StandardTypeDefs.end(), Word) != StandardTypeDefs.end()){
                tokens.push_back(Token{Word, TokenTypes::TypeDef, Line, CurrentLine});
            }
            else if(std::find(ValidMacroStatements.begin(), ValidMacroStatements.end(), Word) != ValidMacroStatements.end()){
                tokens.push_back(Token(Word, TokenTypes::MacroStatement, Line, CurrentLine));
            }
            else if(Word.find_first_not_of("!\"£$%^&*()[]{}#~'@/?.>,<|`¬-") != std::string::npos){
                tokens.push_back(Token{Word, TokenTypes::Identifier, Line, CurrentLine}); // Ascii literals are handled separately
            }
            else{
                Cont = true;
            }
            if(!Cont){
                Stream = EatWord(Stream);
                continue;
            }
        }

        // Check character for non-words
        char c = GetChar(Stream);
        bool actedOnC = false;
        switch (c)
        {
        case '(':
            tokens.push_back(Token{"(", TokenTypes::ArgumentStart, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case ')':
            tokens.push_back(Token{")", TokenTypes::ArgumentEnd, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '{':
            tokens.push_back(Token{"{", TokenTypes::BlockStart, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '}':
            tokens.push_back(Token{"}", TokenTypes::BlockEnd, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '[':
            tokens.push_back(Token{"[", TokenTypes::IndexStart, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case ']':
            tokens.push_back(Token{"]", TokenTypes::IndexEnd, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case ';':
            tokens.push_back(Token{";", TokenTypes::LineEnd, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case ',':
            tokens.push_back(Token{",", TokenTypes::ArgumentSeparator, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '.':
            tokens.push_back(Token{".", TokenTypes::Operator, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '+':
            tokens.push_back(Token{"+", TokenTypes::Operator, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '-':
            tokens.push_back(Token{"-", TokenTypes::Operator, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '*':
            tokens.push_back(Token{"*", TokenTypes::Operator, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '/': // Edge cases of "//" and "/*"
            {
                Stream = EatChar(Stream);
                char c2 = GetChar(Stream);
                if (c2 == '/')
                {
                    Stream = EatChar(Stream);
                    Stream = EatUntil(Stream, std::string("\n"), &Line);
                }
                else if (c2 == '*')
                {
                    Stream = EatChar(Stream);
                    Stream = EatUntil(Stream, std::string("*/"), &Line);
                }
                else
                {
                    tokens.push_back(Token{"/", TokenTypes::Operator, Line, CurrentLine});
                }
            }
            break;
        case '%':
            tokens.push_back(Token{"%", TokenTypes::Operator, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '=':
            {
                // Edge case of next char being "="
                Stream = EatChar(Stream);
                if (GetChar(Stream) == '=')
                {
                    tokens.push_back(Token{"==", TokenTypes::Comparitor, Line, CurrentLine});
                    Stream = EatChar(Stream);
                    break;
                }
                tokens.push_back(Token{"=", TokenTypes::Operator, Line, CurrentLine});
                Stream = EatChar(Stream);
            }
            break;
        case '<':
            // Edge case of next char being "="
            Stream = EatChar(Stream);
            if (GetChar(Stream) == '=')
            {
                tokens.push_back(Token{"<=", TokenTypes::Comparitor, Line, CurrentLine});
                Stream = EatChar(Stream);
                break;
            }
            else if (GetChar(Stream) == '<'){
                tokens.push_back(Token{"<<", TokenTypes::Operator, Line, CurrentLine});
                Stream = EatChar(Stream);
                break;
            }
            tokens.push_back(Token{"<", TokenTypes::Comparitor, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '>':
            // Edge case of next char being "="
            Stream = EatChar(Stream);
            if (GetChar(Stream) == '=')
            {
                tokens.push_back(Token{">=", TokenTypes::Comparitor, Line, CurrentLine});
                Stream = EatChar(Stream);
                break;
            }
            else if (GetChar(Stream) == '>'){
                tokens.push_back(Token{">>", TokenTypes::Operator, Line, CurrentLine});
                Stream = EatChar(Stream);
                break;
            }
            tokens.push_back(Token{">", TokenTypes::Operator, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '!':
            // Edge case of next char being "="
            Stream = EatChar(Stream);
            if (GetChar(Stream) == '=')
            {
                tokens.push_back(Token{"!=", TokenTypes::Comparitor, Line, CurrentLine});
                Stream = EatChar(Stream);
                break;
            }
            tokens.push_back(Token{"!", TokenTypes::Operator, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '&':
            // Edge case of next char being "&"
            Stream = EatChar(Stream);
            if (GetChar(Stream) == '&')
            {
                tokens.push_back(Token{"&&", TokenTypes::BooleanOperator, Line, CurrentLine});
                Stream = EatChar(Stream);
                break;
            }
            tokens.push_back(Token{"&", TokenTypes::Operator, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '|':
            // Edge case of next char being "|"
            Stream = EatChar(Stream);
            if (GetChar(Stream) == '|')
            {
                tokens.push_back(Token{"||", TokenTypes::BooleanOperator, Line, CurrentLine});
                Stream = EatChar(Stream);
                break;
            }
            tokens.push_back(Token{"|", TokenTypes::Operator, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        case '^':
            break;
        case '~': // Bitwise or Deconstructor
            break;
        case '?':
            break;
        case ':':
            break;
        case '"': // Needs closing
            {
                Stream = EatChar(Stream);
                std::string str = "\"" + GetUntil(Stream, '"') + "\"";
                tokens.push_back(Token{str, TokenTypes::Literal, Line, CurrentLine});
                Stream = EatUntil(Stream, '"', &Line);
            }
            break;
        case '\'': // Needs closing
            break;
        case '\\': // Invalid
            break;
        case '#':
            // TODO #Ifdef ...
            tokens.push_back(Token{"#", TokenTypes::Macro, Line, CurrentLine});
            Stream = EatChar(Stream);
            break;
        default:
            break;
        }
    }
}
    