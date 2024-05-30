#pragma once

#include <Parsing/Tokens.h>

#include <string>
#include <vector>

namespace Parsing{
    class LexicalAnalyser{
    protected:
        std::vector<Token> tokens = {};
        const std::string AssociatedFile = "";

    public: // Constructors
        LexicalAnalyser(const std::string& Data, const std::string FilePath);

    public: // Methods
        const std::vector<Token>& Tokens() {return tokens;};
        const Token& getToken(const int& tokenIndex) {static Token NullToken = {"", TokenTypes::NullToken, -1}; if (tokenIndex >= tokens.size()) return NullToken; return tokens[tokenIndex];};
        inline const char* GetFilename() {return AssociatedFile.data();}
    };
}