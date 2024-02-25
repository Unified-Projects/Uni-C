#include <Parsing/LexicalAnalyser.h>
#include <Parsing/UtilityFunctions.h>

#include <algorithm>

using namespace Parsing;

// Statements
std::vector<std::string> ValidStatements = {
    "if",
    "else",
    "for",
    "while",
    "do",
    "switch",
    "case",
    "default",
    "break",
    "continue",
    "return",
    "goto",
    "asm",
    "try",
    "catch",
    "throw",
    "using",
    "typedef",
    "enum",
    "struct",
    "union",
    "class",
    "namespace",
    "template",
    "typename",
    "auto",
    "register",
    "static",
    "extern",
    "mutable",
    "inline",
    "virtual",
    "explicit",
    "friend",
    "public",
    "protected",
    "private",
    "const",
    "volatile",
    "alignof",
    "decltype",
    "noexcept",
    "nullptr",
    "sizeof",
    "typeid"
};

std::vector<std::string> StandardTypeDefs = {
    "int",
    "uint",
    "short",
    "ushort",
    "long",
    "ulong",
    "float",
    "double",
    "char",
    "uchar",
    "bool",
    "void",
};

LexicalAnalyser::LexicalAnalyser(const std::string& Data){
    // Iterate over data
    std::string Stream = Data;

    while (Data.size() > 0)
    {
        Stream = EatWhitespace(Stream);

        if(Stream.size() == 0){
            break;
        }

        // Check character for non-words
        char c = GetChar(Stream);
        bool actedOnC = false;
        switch (c)
        {
        case '(':
            tokens.push_back(Token{"(", TokenTypes::ArgumentStart});
            Stream = EatChar(Stream);
            break;
        case ')':
            tokens.push_back(Token{")", TokenTypes::ArgumentEnd});
            Stream = EatChar(Stream);
            break;
        case '{':
            tokens.push_back(Token{"{", TokenTypes::BlockStart});
            Stream = EatChar(Stream);
            break;
        case '}':
            tokens.push_back(Token{"{", TokenTypes::BlockEnd});
            Stream = EatChar(Stream);
            break;
        case '[':
            tokens.push_back(Token{"[", TokenTypes::IndexStart});
            Stream = EatChar(Stream);
            break;
        case ']':
            tokens.push_back(Token{"]", TokenTypes::IndexEnd});
            Stream = EatChar(Stream);
            break;
        case ';':
            tokens.push_back(Token{";", TokenTypes::LineEnd});
            Stream = EatChar(Stream);
            break;
        case ',':
            break;
        case '.':
            break;
        case '+':
            break;
        case '-':
            break;
        case '*':
            break;
        case '/': // Edge cases of "//" and "/*"
            break;
        case '%':
            break;
        case '=':
            break;
        case '<':
            break;
        case '>':
            break;
        case '!':
            break;
        case '&':
            break;
        case '|':
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
            break;
        case '\'': // Needs closing
            break;
        case '\\': // Invalid
            break;
        default:
            break;
        }
        
        Stream = EatWhitespace(Stream);

        if(Stream.size() == 0){
            break;
        }

        // Check for words
        std::string Word = GetWord(Stream);
        if(Word.size() > 0){
            if(std::find(ValidStatements.begin(), ValidStatements.end(), Word) != ValidStatements.end()){
                tokens.push_back(Token{Word, TokenTypes::Statement});
            }
            else if(std::find(StandardTypeDefs.begin(), StandardTypeDefs.end(), Word) != StandardTypeDefs.end()){
                tokens.push_back(Token{Word, TokenTypes::TypeDef});
            }
            else{
                if(Word.find_first_not_of("0123456789.-") == std::string::npos && Word.find('-') == Word[0] && std::count(Word.begin(), Word.end(), '.') <= 1){
                    tokens.push_back(Token{Word, TokenTypes::Literal});
                }
                else{
                    tokens.push_back(Token{Word, TokenTypes::Identifier}); // Ascii literals are handled separately
                }
            }
            Stream = EatWord(Stream);
            continue;
        }
    }
}
    