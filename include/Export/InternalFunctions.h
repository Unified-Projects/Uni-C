#pragma once

#include <string>
#include <map>
#include <vector>

struct CustomFunctionDeclaration{
    std::string FunctionName;
    std::vector<std::string> Externs;
    std::string BSS;
    std::string DATA;
    std::string Function;
};

extern std::map<std::string, CustomFunctionDeclaration> ExternalFunctions;