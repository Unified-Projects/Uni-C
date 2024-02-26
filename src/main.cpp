#include <InputStream/Configurator.h>
#include <InputStream/File.h>
#include <Parsing/LexicalAnalyser.h>
#include <Parsing/SemanticAnalyser.h>
#include <Export/AssemblyGenerator.h>
#include <CompilerInformation.h>

#include <iostream>
#include <map>
#include <functional>

// Pull External Debug Inforamtion
extern std::map<std::string, Parsing::SemanticFunctionDeclaration> SemanticFunctionMap;
extern std::map<std::string, Parsing::SemanticTypeDefinition> SemanticTypeMap;

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

    // Semantic Analysis
    Parsing::SemanticAnalyser* semanticAnalyser = new Parsing::SemanticAnalyser(analysers);
    if(semanticAnalyser->getRootScope() == nullptr){
        std::cerr << "Failed to compile!" << std::endl;
        return -1;
    }
    if(CompilerInformation::DebugAll()){
        std::cout << "-------- Semantic Analysis --------" << std::endl;
        std::cout << "Types: " << std::endl;
        for(auto& type : SemanticTypeMap){
            if(type.second.Scope < 0) continue;
            std::cout << "  Name: " << type.first << " Scope: " << type.second.Scope << " Size: " << type.second.Size << std::endl;
        }
        std::cout << "Functions: " << std::endl;
        for(auto& func : SemanticFunctionMap){
            if(func.second.Scope < 0) continue;
            std::cout << "  Name: " << func.first << " Scope: " << func.second.Scope << std::endl;
        }

        std::cout << "Tree: " << std::endl;

        // Tree traverse block chain from root scope printing out types
        std::function<void(Parsing::SemanticVariables::SemanticVariable*, int)> traverse = [&](Parsing::SemanticVariables::SemanticVariable* scope, int depth){
            // Log all Semantice Variables for the specific type using casting then traverse children if present
            if(scope->Type() == Parsing::SemanticVariables::SemanticTypes::SemanticTypeType){
                Parsing::SemanticVariables::SemTypeDef* type = (Parsing::SemanticVariables::SemTypeDef*)scope;
                std::cout << std::string(depth * 2, ' ') << "TypeDef: " << type->Identifier << " Type: " << type->TypeID << " Scope: " << type->LocalScope << ":" << type->ScopePosition << " Size: " << type->Size << std::endl;

                std::cout << std::string((depth + 1) * 2, ' ') << "Attributes: " << std::endl;
                for(auto& attr : type->Attributes){
                    traverse(attr, depth + 2);
                }

                std::cout << std::string((depth + 1) * 2, ' ') << "Methods: " << std::endl;
                for(auto& attr : type->Methods){
                    traverse(attr, depth + 2);
                }
            }
            if(scope->Type() == Parsing::SemanticVariables::SemanticTypes::SemanticTypeVariable){
                Parsing::SemanticVariables::Variable* type = (Parsing::SemanticVariables::Variable*)scope;
                std::cout << std::string(depth * 2, ' ') << "Variable: " << type->Identifier << " Type: " << type->TypeID << " Init: " << type->InitValue << " Scope: " << type->LocalScope << ":" << type->ScopePosition << std::endl;
            }
            if(scope->Type() == Parsing::SemanticVariables::SemanticTypes::SemanticTypeLiteral){
                Parsing::SemanticVariables::SemLiteral* type = (Parsing::SemanticVariables::SemLiteral*)scope;
                std::cout << std::string(depth * 2, ' ') << "Value: " << type->Value << " Type: " << type->TypeID << " Scope: " << type->LocalScope << ":" << type->ScopePosition << std::endl;
            }
            if(scope->Type() == Parsing::SemanticVariables::SemanticTypes::SemanticTypeVariableRef){
                Parsing::SemanticVariables::VariableRef* type = (Parsing::SemanticVariables::VariableRef*)scope;
                std::cout << std::string(depth * 2, ' ') << "Reference of: " << type->Identifier << " Scope: " << type->LocalScope << ":" << type->ScopePosition << std::endl;
            }
            if(scope->Type() == Parsing::SemanticVariables::SemanticTypes::SemanticTypeFunction){
                Parsing::SemanticVariables::Function* func = (Parsing::SemanticVariables::Function*)scope;
                std::cout << std::string(depth * 2, ' ') << "Function: " << func->Identifier << " Scope: " << func->LocalScope << ":" << func->ScopePosition << std::endl;

                std::cout << std::string((depth + 1) * 2, ' ') << "Parameters: " << std::endl;
                for(auto& child : func->Parameters){
                    traverse(child, depth + 2);
                }
                std::cout << std::string((depth + 1) * 2, ' ') << "Block: " << std::endl;
                for(auto& child : func->Block){
                    traverse(child, depth + 2);
                }
            }
            if(scope->Type() == Parsing::SemanticVariables::SemanticTypes::SemanticTypeScope){
                Parsing::SemanticVariables::Scope* scopeVar = (Parsing::SemanticVariables::Scope*)scope;
                std::cout << std::string(depth * 2, ' ') << "Scope: " << scopeVar->LocalScope << ":" << scopeVar->ScopePosition << std::endl;
                for(auto& child : scopeVar->Block){
                    traverse(child, depth + 1);
                }
            }
            if(scope->Type() == Parsing::SemanticVariables::SemanticTypes::SemanticTypeStatement){
                Parsing::SemanticVariables::SemStatement* statement = (Parsing::SemanticVariables::SemStatement*)scope;
                std::cout << std::string(depth * 2, ' ') << "Statement: " << statement->TypeID << " Scope: " << statement->LocalScope << ":" << statement->ScopePosition << std::endl;
                std::cout << std::string((depth + 1) * 2, ' ') << "Parameters: " << std::endl;
                for(auto& child : statement->Parameters){
                    traverse(child, depth + 2);
                }
                std::cout << std::string((depth + 1) * 2, ' ') << "Block: " << std::endl;
                for(auto& child : statement->Block){
                    traverse(child, depth + 2);
                }
            }
        };
        traverse(semanticAnalyser->getRootScope(), 0);
        std::cout << "------ End Semantic Analysis ------" << std::endl;
    }

    // Start Generation
    Exporting::AssemblyGenerator* generator = new Exporting::AssemblyGenerator();
    std::string assembly = generator->Generate(semanticAnalyser->getRootScope());

    // Write to file
    {
        InputStream::FileHandler output_file = InputStream::FileHandler(InputStream::__COMPILATION_CONFIGURATION->OutputFile());
        output_file = assembly;
        output_file.Close();
    }

    std::cout << "Compilation Successful! Output: " << InputStream::__COMPILATION_CONFIGURATION->OutputFile() << std::endl;

    return 0;
}