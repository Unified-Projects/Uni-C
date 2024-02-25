#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <string>

namespace InputStream
{
    class Configuration{
    protected: // Variables
        // File handling
        std::vector<const char*>* input_files = nullptr;
        const char* output_file = "";

        // Linking
        std::vector<const char*>* include_directories = nullptr;

        // Compilation
        int SyntaxVersion = 0;
        int OptimiserLevel = 0;

        // Definitions
        std::map<std::string, std::string> definitions = {};

        // Logger
        /*
            Bit Flask Values:
            0x00000001 - Log to file
            0x00000002 - Quiet Log to console
            0x00000004 - Ignore Warnings
        */
        int ErrorFlags = 0; // (BitMask)

    public: // Variables
        class ArgumentParser* args = nullptr;
    
    public: // Constructors
        Configuration(int argc, char** argv);
        ~Configuration();

    public: // Methods
        // File Handling
        const std::vector<const char*>& InputFiles() const { return *input_files; }
        const char*& OutputFile() { return output_file; }

        // Linking
        const std::vector<const char*>& IncludeDirectories() const { return *include_directories; }

        // Compilation
        const int& GetSyntaxVersion() const { return SyntaxVersion; }
        const int& GetOptimiserLevel() const { return OptimiserLevel; }

        // Definitions
        const std::map<std::string, std::string>& Definitions() const { return definitions; }

        // Logger
        const int& GetErrorFlags() const { return ErrorFlags; }
    };

    extern Configuration* __COMPILATION_CONFIGURATION;
} // namespace InputStream
