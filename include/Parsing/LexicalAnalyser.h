#pragma once

#include <Parsing/Tokens.h>

#include <string>
#include <vector>

namespace Parsing{
    class LexicalAnalyser{
    protected:
        std::vector<Token> tokens = {};

    public: // Constructors
        LexicalAnalyser(const std::string& Data);

    public: // Methods
        const std::vector<Token>& Tokens() {return tokens;};
        const Token& getToken(const int& tokenIndex) {return tokens[tokenIndex];};
    };
}