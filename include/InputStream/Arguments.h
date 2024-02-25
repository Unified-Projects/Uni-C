#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

namespace InputStream{
    class ArgumentParser{
    protected:
        int argc;
        char** argv;

        // Tokens
        std::vector<std::string> tokens;

    public: // Constructors
        ArgumentParser(int argc, char** argv);

    public: // Methods
        bool hasToken(const std::string& token);
        const std::string& getTokenValue(const std::string& token);
        const std::string& getToken(const int& tokenIndex);
        const int getTokenIndex(const std::string& token);
        const int getTokenType(const std::string& token); // 0 = No Token, 1 = -X, 2 = Token=Value, 3 = Value
        const int getTokenType(const int& tokenIndex); // 0 = No Token, 1 = -X, 2 = Token=Value, 3 = Value
    };
}