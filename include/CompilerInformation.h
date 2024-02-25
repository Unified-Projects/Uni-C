#pragma once

namespace CompilerInformation
{
    static inline int SyntaxVersion() { return 1; }
    static inline int StandardOptimiserLevel() { return 0; }
    static inline int StandardErrorFlags() { return 0; }

    static inline const char* CompilerName() { return "Unified-Projects Uni-C Compiler"; }
    static inline const char* CompilerVersionName() { return "Under Development"; }
    static inline const char* CompilerVersion() { return "0.0.1"; }

    static inline int DebugAll() { return 1; }
} // namespace COMPILER_STANDARDS`
