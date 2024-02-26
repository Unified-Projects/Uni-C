#pragma once

#include <string>
#include <cstring>

namespace Parsing
{
    std::string EatWhitespace(const std::string& str, int* newLines);
    std::string EatChar(const std::string& str);
    std::string EatWord(const std::string& str, bool i = false);
    std::string EatUntil(const std::string& str, const char& end);

    std::string GetWord(const std::string& str, bool i = false);
    char GetChar(const std::string& str);
    std::string GetUntil(const std::string& str, const char& end);
} // namespace Parsing
