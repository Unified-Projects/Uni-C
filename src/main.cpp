#include <InputStream/Configurator.h>
#include <InputStream/File.h>
#include <Parsing/LexicalAnalyser.h>
#include <CompilerInformation.h>

#include <iostream>

int main(int argc, char* argv[]){
    // Loac Configuration
    InputStream::Configuration(argc, argv);

    // Ensure required configuration is present
    if(InputStream::__COMPILATION_CONFIGURATION->InputFiles().size() == 0){
        std::cerr << "No input files provided" << std::endl;
        return 1;
    }

    if(InputStream::__COMPILATION_CONFIGURATION->OutputFile() == nullptr){
        std::cerr << "No output file provided" << std::endl;
        return 1;
    }

    if(CompilerInformation::DebugAll()){
        std::cout << "-------- Configuration --------" << std::endl;
        // Debug Config
        std::cout << "Input Files: " << std::endl;
        for(auto& file : InputStream::__COMPILATION_CONFIGURATION->InputFiles()){
            std::cout << "  " << file << std::endl;
        }

        std::cout << "Output File: " << InputStream::__COMPILATION_CONFIGURATION->OutputFile() << std::endl;

        std::cout << "Include Directories: " << std::endl;
        for(auto& dir : InputStream::__COMPILATION_CONFIGURATION->IncludeDirectories()){
            std::cout << "  " << dir << std::endl;
        }

        std::cout << "Syntax Version: " << InputStream::__COMPILATION_CONFIGURATION->GetSyntaxVersion() << std::endl;
        std::cout << "Optimiser Level: " << InputStream::__COMPILATION_CONFIGURATION->GetOptimiserLevel() << std::endl;

        std::cout << "Definitions: " << std::endl;
        for(auto& def : InputStream::__COMPILATION_CONFIGURATION->Definitions()){
            std::cout << "  " << def.first << " = " << def.second << std::endl;
        }

        std::cout << "Error Flags: " << InputStream::__COMPILATION_CONFIGURATION->GetErrorFlags() << std::endl;

        std::cout << "------ End Configuration ------" << std::endl;
    }

    // Apply Lexical Analysis On Input Files
    std::vector<Parsing::LexicalAnalyser*> analysers;
    for(auto& file : InputStream::__COMPILATION_CONFIGURATION->InputFiles()){
        InputStream::FileHandler input_file = InputStream::FileHandler(file);
        Parsing::LexicalAnalyser* analyser = new Parsing::LexicalAnalyser(std::string(input_file));
        analysers.push_back(analyser);

        // Debugger
        if(CompilerInformation::DebugAll()){
            std::cout << "-------- Lexical Analysis --------" << std::endl;
            std::cout << "File: " << file << std::endl;
            std::cout << "Tokens: " << std::endl;
            for(auto& token : analyser->Tokens()){
                std::cout << "Value: " << token.tokenValue << " Type: " << token.tokenType << std::endl;
            }
            std::cout << "------ End Lexical Analysis ------" << std::endl;
        }
    }

    return 0;
}