#pragma once

#include <string>

namespace Parsing
{
    enum TokenTypes{
        NullToken,
        TypeDef,
        Identifier,
        Operator,
        ArgumentStart,
        ArgumentEnd,
        ArgumentSeparator,
        IndexStart,
        IndexEnd,
        LineEnd,
        BlockStart,
        BlockEnd,
        LineComment,
        CommentBlockStart,
        CommentBlockEnd,
        Literal,
        Statement // (for, while, if, else, delete, new, opp, return, break, continue, switch, case, default, goto, try, catch, throw, using, namespace, class, struct, enum, typedef, template, friend, public, protected, private, virtual, static, const, volatile, inline, extern, register, mutable, explicit, export, operator, sizeof, typeid, alignof, decltype, noexcept, nullptr, true, false, asm, auto, bool, char, wchar_t, char8_t, char16_t, char32_t, short, int, long, float, double, void, signed, unsigned, const, volatile, inline, extern, register, mutable, explicit, export, operator, sizeof, typeid, alignof, decltype, noexcept, nullptr, true, false, asm, auto, bool, char, wchar_t, char8_t, char16_t, char32_t, short, int, long, float, double, void, signed, unsigned, const, volatile, inline, extern, register, mutable, explicit, export, operator, sizeof, typeid, alignof, decltype, noexcept, nullptr, true, false, asm, auto, bool, char, wchar_t, char8_t, char16_t, char32_t, short, int, long, float, double, void, signed, unsigned, const, volatile, inline, extern, register, mutable, explicit, export, operator, sizeof, typeid, alignof, decltype, noexcept, nullptr, true, false, asm, auto, bool, char, wchar_t, char8_t, char16_t, char32_t, short, int, long, float, double, void, signed, unsigned, const, volatile, inline, extern, register, mutable, explicit, export, operator, sizeof, typeid, alignof, decltype, noexcept, nullptr, true, false, asm, auto, bool, char, wchar_t, char8_t, char16_t, char32_t, short, int, long, float, double, void, signed, unsigned, const, volatile, inline, extern, register, mutable, explicit, export, operator, sizeof, typeid, alignof, decltype, noexcept, nullptr, true, false, asm, auto, bool, char, wchar_t, char8_t, char16_t, char32_t, short, int, long, float, double, void, signed, unsigned, const, volatile, inline, extern, register)
    };

    struct Token
    {
        std::string tokenValue = "";
        TokenTypes tokenType = Literal;
        int fileLine = 0;
        std::string tokenFileLine = "";
    };
    
} // namespace Parsing
