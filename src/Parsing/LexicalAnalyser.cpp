#include <Parsing/LexicalAnalyser.h>
#include <Parsing/UtilityFunctions.h>

#include <algorithm>

#include <iostream> // TODO Remove

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
    "string"
};

LexicalAnalyser::LexicalAnalyser(const std::string& Data){
    // Iterate over data
    std::string Stream = Data;
    int Line = 1;

    while (Data.size() > 0)
    {
        Stream = EatWhitespace(Stream, &Line);

        if(Stream.size() == 0){
            break;
        }

        Stream = EatWhitespace(Stream, &Line);
 
        if(Stream.size() == 0){
            break;
        }

        // Check for words
        std::string Word = GetWord(Stream);
        std::string WordI = GetWord(Stream, true);
        if(WordI.size() > 0 && WordI.find_first_not_of("0123456789.-") == std::string::npos && ('-' == WordI.front() || WordI.find('-') == std::string::npos) && std::count(WordI.begin(), WordI.end(), '.') <= 1 && std::count(WordI.begin(), WordI.end(), '-') <= 1 && (std::count(WordI.begin(), WordI.end(), '.') + std::count(WordI.begin(), WordI.end(), '-') < WordI.size())){
            tokens.push_back(Token{WordI, TokenTypes::Literal, Line});
            Stream = EatWord(Stream, true);
        }
        else if(Word.size() > 0){
            if(std::find(ValidStatements.begin(), ValidStatements.end(), Word) != ValidStatements.end()){
                tokens.push_back(Token{Word, TokenTypes::Statement, Line});
            }
            else if(std::find(StandardTypeDefs.begin(), StandardTypeDefs.end(), Word) != StandardTypeDefs.end()){
                tokens.push_back(Token{Word, TokenTypes::TypeDef, Line});
            }
            else{
                tokens.push_back(Token{Word, TokenTypes::Identifier, Line}); // Ascii literals are handled separately
            }
            Stream = EatWord(Stream);
            continue;
        }

        // Check character for non-words
        char c = GetChar(Stream);
        bool actedOnC = false;
        switch (c)
        {
        case '(':
            tokens.push_back(Token{"(", TokenTypes::ArgumentStart, Line});
            Stream = EatChar(Stream);
            break;
        case ')':
            tokens.push_back(Token{")", TokenTypes::ArgumentEnd, Line});
            Stream = EatChar(Stream);
            break;
        case '{':
            tokens.push_back(Token{"{", TokenTypes::BlockStart, Line});
            Stream = EatChar(Stream);
            break;
        case '}':
            tokens.push_back(Token{"}", TokenTypes::BlockEnd, Line});
            Stream = EatChar(Stream);
            break;
        case '[':
            tokens.push_back(Token{"[", TokenTypes::IndexStart, Line});
            Stream = EatChar(Stream);
            break;
        case ']':
            tokens.push_back(Token{"]", TokenTypes::IndexEnd, Line});
            Stream = EatChar(Stream);
            break;
        case ';':
            tokens.push_back(Token{";", TokenTypes::LineEnd, Line});
            Stream = EatChar(Stream);
            break;
        case ',':
            tokens.push_back(Token{",", TokenTypes::ArgumentSeparator, Line});
            Stream = EatChar(Stream);
            break;
        case '.':
            tokens.push_back(Token{".", TokenTypes::Operator, Line});
            Stream = EatChar(Stream);
            break;
        case '+':
            tokens.push_back(Token{"+", TokenTypes::Operator, Line});
            Stream = EatChar(Stream);
            break;
        case '-':
            tokens.push_back(Token{"-", TokenTypes::Operator, Line});
            Stream = EatChar(Stream);
            break;
        case '*':
            tokens.push_back(Token{"*", TokenTypes::Operator, Line});
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
                    tokens.push_back(Token{"/", TokenTypes::Operator, Line});
                }
            }
            break;
        case '%':
            tokens.push_back(Token{"%", TokenTypes::Operator, Line});
            Stream = EatChar(Stream);
            break;
        case '=':
            {
                // Edge case of next char being "="
                Stream = EatChar(Stream);
                if (GetChar(Stream) == '=')
                {
                    tokens.push_back(Token{"==", TokenTypes::Operator, Line});
                    Stream = EatChar(Stream);
                    break;
                }
                tokens.push_back(Token{"=", TokenTypes::Operator, Line});
                Stream = EatChar(Stream);
            }
            break;
        case '<':
            break;
        case '>':
            break;
        case '!':
            tokens.push_back(Token{"!", TokenTypes::Operator, Line});
            Stream = EatChar(Stream);
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
            {
                Stream = EatChar(Stream);
                std::string str = "\"" + GetUntil(Stream, '"') + "\"";
                tokens.push_back(Token{str, TokenTypes::Literal, Line});
                Stream = EatUntil(Stream, '"', &Line);
            }
            break;
        case '\'': // Needs closing
            break;
        case '\\': // Invalid
            break;
        default:
            break;
        }
    }
}
    