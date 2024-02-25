#include <InputStream/Configurator.h>
#include <InputStream/Arguments.h>
#include <CompilerInformation.h>

using namespace InputStream;

Configuration* InputStream::__COMPILATION_CONFIGURATION = nullptr;

Configuration::Configuration(int argc, char** argv){
    if(!__COMPILATION_CONFIGURATION){ // Load Globally
        __COMPILATION_CONFIGURATION = this;}

    args = new ArgumentParser(argc, argv);

    // Reset variables
    input_files = new std::vector<const char*>();
    output_file = nullptr;
    include_directories = new std::vector<const char*>();
    SyntaxVersion = 0;
    OptimiserLevel = 0;
    definitions = {};
    ErrorFlags = 0;
    
    // Expected Format
    // -i (Set of input files spaced out)
    // -o (Output file name)
    // -I (Include Directory)
    // -S= (Syntax Version)
    // -O= (Optimiser Level)
    // -D (Example -DDefinition=1)
    // -E= (Error Flags as a integer)

    if (args->hasToken("-i")) {
        int TokenIndex = args->getTokenIndex("-i");

        // Clone each file
        while (args->getTokenType(TokenIndex + 1) == 1) { // While the next token is a value
            std::string inputFile = args->getToken(TokenIndex + 1);
            input_files->push_back(strdup(inputFile.c_str())); // Directly push the string into the vector
            TokenIndex++;
        }
    }

    if (args->hasToken("-o")) {
        int TokenIndex = args->getTokenIndex("-o");

        // Clone output file
        if (args->getTokenType(TokenIndex + 1) == 1) { // While the next token is a value
            output_file = strdup(args->getToken(TokenIndex + 1).c_str());
        }
    }

    if (args->hasToken("-I")) {
        int TokenIndex = args->getTokenIndex("-I");

        // Clone each directory
        while (args->getTokenType(TokenIndex + 1) == 1) { // While the next token is a value
            std::string includeDir = args->getToken(TokenIndex + 1);
            include_directories->push_back(strdup(includeDir.c_str()));
            TokenIndex++;
        }
    }

    if(args->hasToken("-S")){
        int TokenIndex = args->getTokenIndex("-S");

        // Load each file
        if(args->getTokenType(TokenIndex) == 2){ // While the next token is a value
            SyntaxVersion = std::stoi(args->getTokenValue("-S"));
        }
        else{
            SyntaxVersion = CompilerInformation::SyntaxVersion();
        }
    } else{
        SyntaxVersion = CompilerInformation::SyntaxVersion();
    }
    
    if(args->hasToken("-O")){
        int TokenIndex = args->getTokenIndex("-O");

        // Load each file
        if(args->getTokenType(TokenIndex) == 2){ // While the next token is a value
            OptimiserLevel = std::stoi(args->getTokenValue("-O"));
        }
        else{
            OptimiserLevel = CompilerInformation::StandardOptimiserLevel();
        }
    } else{
        OptimiserLevel = CompilerInformation::StandardOptimiserLevel();
    }

    int i = 0;
    while(true){
        // Check Token Is Valid
        if(args->getTokenType(i) == 0){
            break;
        }

        // Get Token
        std::string Token = args->getToken(i);

        // Check if it is a definition
        if(Token.find("-D") == 0){
            // Split the definition
            std::string Definition = Token.substr(2);
            std::string Value = "";
            if(Definition.find('=') != std::string::npos){
                Value = Definition.substr(Definition.find('=') + 1);
                Definition = Definition.substr(0, Definition.find('='));
            }

            // Add to definitions
            definitions[Definition] = std::move(Value);
        }

        i++;
    }

    if(args->hasToken("-E")){
        int TokenIndex = args->getTokenIndex("-E");

        // Load each file
        if(args->getTokenType(TokenIndex) == 2){ // While the next token is a value
            ErrorFlags = std::stoi(args->getTokenValue("-E"));
        }
        else{
            ErrorFlags = CompilerInformation::StandardErrorFlags();
        }
    }
    else{
        ErrorFlags = CompilerInformation::StandardErrorFlags();
    }

    return;
}

Configuration::~Configuration(){
    delete args;
}