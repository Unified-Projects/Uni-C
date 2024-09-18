#include <InputStream/Configurator.h>
#include <InputStream/File.h>
#include <Parsing/LexicalAnalyser.h>
#include <Parsing/SemanticAnalyser.h>
#include <Export/AssemblyGenerator.h>
#include <CompilerInformation.h>

#include <iostream>
#include <map>
#include <functional>

std::vector<Parsing::LexicalAnalyser*> analysers;

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
    for(auto& file : InputStream::__COMPILATION_CONFIGURATION->InputFiles()){
        InputStream::FileHandler input_file = InputStream::FileHandler(file);
        Parsing::LexicalAnalyser* analyser = new Parsing::LexicalAnalyser(std::string(input_file), file);
        analysers.push_back(analyser);

        // Debugger
        if(CompilerInformation::DebugAll()){
            std::cout << "-------- Lexical Analysis --------" << std::endl;
            std::cout << "File: " << file << std::endl;
            std::cout << "Tokens: " << std::endl;
            int TokenI = 0;
            for(auto& token : analyser->Tokens()){
                std::cout << "Value: " << token.tokenValue << " Type: " << token.tokenType << " Index: " << TokenI++ << std::endl;
            }
            std::cout << "------ End Lexical Analysis ------" << std::endl;
        }
    }

    // Semantic Analysis
    Parsing::SemanticAnalyser* semanticAnalyser = new Parsing::SemanticAnalyser(analysers);

    const int IndentCount = 3;

    std::function<void(Parsing::SemanticVariables::SemanticBlock*, int)> LogBlock = [&](Parsing::SemanticVariables::SemanticBlock* Block, int depth){
        std::cout << std::string(depth * IndentCount, ' ') << "Block: " << std::endl;
        
        // Variables
        if(Block->Variables.size() > 0){
            std::cout << std::string(depth*IndentCount + IndentCount, ' ') << "Variables: " << std::endl;
            for(auto p : Block->Variables){
                std::cout << std::string(depth*IndentCount + IndentCount*2, ' ') << p->Identifier << ": t(" << p->TypeDef << ")" << ((p->Initialiser.size() > 0) ? ("t(" + p->Initialiser + ")") : "") << std::endl;
            }
        }

        // Block
        int IdentPlus = 0;
        if(Block->Variables.size() > 0){
            std::cout << std::string(depth*IndentCount + IndentCount, ' ') << "Block: " << std::endl;
            IdentPlus++;
        }
        
        for(auto x : Block->Block){
            if(x->GetType() == 0){
                // Statement
                auto SemStat = (Parsing::SemanticVariables::SemanticStatment*)x;
                std::cout << std::string((depth + 1 + IdentPlus)*(IndentCount), ' ') << "Statement: t(" << SemStat->StateType << ")" << std::endl;

                for(auto p : SemStat->ParameterVariables){
                    std::cout << std::string((depth + 2 + IdentPlus)*(IndentCount), ' ') << "Variable: " << " t(" << p->TypeDef << ") v(" << p->Initialiser << ")" << std::endl;
                }
                for(auto p : SemStat->ParameterOperations){
                    auto ParOP = (Parsing::SemanticVariables::SemanticOperation*)p;
                    std::cout << std::string((depth + 2 + IdentPlus)*(IndentCount), ' ') << "Operation: t(" << ParOP->EvaluatedTypedef << ")" << std::endl;
                    for(auto o : ParOP->Operations){
                        std::cout << std::string((depth + 3 + IdentPlus)*(IndentCount), ' ') << "t(" << o->Type << ") T(" << o->TypeDef << ") v(" << o->Value << ")" << std::endl;
                    }
                }
                std::cout << std::string((depth + 2 + IdentPlus)*(IndentCount), ' ') << "Conditions:" << std::endl;
                for(auto p : SemStat->ParameterConditions){
                    auto ParConC = (Parsing::SemanticVariables::SemanticCondition*)p;
                    auto ParConB = (Parsing::SemanticVariables::SemanticBooleanOperator*)p;
                    if(p->GetType() == 3){
                        std::cout << std::string((depth + 3 + IdentPlus)*(IndentCount), ' ') << "Comparison: T(" << ParConC->Condition << ")" << std::endl;

                        auto SemOp = (Parsing::SemanticVariables::SemanticOperation*)ParConC->Operation1;
                        std::cout << std::string((depth + 4 + IdentPlus)*(IndentCount), ' ') << "Operation 1: t(" << SemOp->EvaluatedTypedef << ")" << std::endl;

                        for(auto o : SemOp->Operations){
                            std::cout << std::string((depth + 5 + IdentPlus)*(IndentCount), ' ') << "t(" << o->Type << ") T(" << o->TypeDef << ") v(" << o->Value << ")" << std::endl;
                        }

                        SemOp = (Parsing::SemanticVariables::SemanticOperation*)ParConC->Operation2;
                        
                        std::cout << std::string((depth + 4 + IdentPlus)*(IndentCount), ' ') << "Operation 2: t(" << SemOp->EvaluatedTypedef << ")" << std::endl;

                        for(auto o : SemOp->Operations){
                            std::cout << std::string((depth + 5 + IdentPlus)*(IndentCount), ' ') << "t(" << o->Type << ") T(" << o->TypeDef << ") v(" << o->Value << ")" << std::endl;
                        }
                    }
                    else if(p->GetType() == 4){
                        std::cout << std::string((depth + 3 + IdentPlus)*(IndentCount), ' ') << "Boolean Operation: T(" << ParConB->Condition << ")" << std::endl;
                    }
                    else{
                        std::cout << std::string((depth + 3 + IdentPlus)*(IndentCount), ' ') << "Compact Operation: Cannot be logged" << std::endl;
                    }
                }

                if(SemStat->Block){
                    LogBlock((Parsing::SemanticVariables::SemanticBlock*)SemStat->Block, depth+2 + IdentPlus);
                }
            }
            else if(x->GetType() == 1){
                auto SemOp = (Parsing::SemanticVariables::SemanticOperation*)x;
                std::cout << std::string((depth + 1 + IdentPlus)*(IndentCount), ' ') << "Operation: t(" << SemOp->EvaluatedTypedef << ")" << std::endl;

                for(auto o : SemOp->Operations){
                    std::cout << std::string((depth + 2 + IdentPlus)*(IndentCount), ' ') << "t(" << o->Type << ") T(" << o->TypeDef << ") v(" << o->Value << ")" << std::endl;
                }
            }
            else if(x->GetType() == 2){
                // Block
                LogBlock((Parsing::SemanticVariables::SemanticBlock*)x, depth+1 + IdentPlus);
            }
        }
    };

    // Log a function definition
    std::function<void(Parsing::SemanticVariables::SemanticFunctionDeclaration*, int)> LogFunctionDefinition = [&](Parsing::SemanticVariables::SemanticFunctionDeclaration* Function, int depth){
        std::cout << std::string(depth * IndentCount, ' ') << Function->Identifier << ": " << ((Function->Private) ? "P" : "") << ((Function->Static) ? "S" : "") << ((Function->IsBuiltin) ? "(Bultin)" : "") << std::endl;

        // Retun
        if(Function->FunctionReturn.WillReturn){
            std::cout << std::string(depth*IndentCount + IndentCount, ' ') << "Function Will Return: " << Function->FunctionReturn.TypeDef << std::endl;
        }
        
        // Parameters
        if(Function->Parameters.size() > 0){
            std::cout << std::string(depth*IndentCount + IndentCount, ' ') << "Parameters: " << std::endl;
            for(auto p : Function->Parameters){
                std::cout << std::string(depth*IndentCount + IndentCount*2, ' ') << p->Identifier << ": t(" << p->TypeDef << ")" << ((p->Initialiser.size() > 0) ? ("t(" + p->Initialiser + ")") : "") << std::endl;
            }
        }

        // Block
        if(Function->Block){
            LogBlock(Function->Block, depth+1);
        }
    };

    // Loop over each loaded file
    if(CompilerInformation::DebugAll())
    std::cout << "-------- Semantic Analysis --------" << std::endl;
    if(CompilerInformation::DebugAll())
    for(auto Interpretation : semanticAnalyser->GetFiles()){
        // std::cout << std::string(IndentCount, ' ') <<  << std::endl;

        std::cout << "File: " << Interpretation->AssociatedFile << std::endl;

        // Linked Files
        if(Interpretation->Associations.size() > 0){    
            std::cout << std::string(IndentCount, ' ') << "Associations: " << std::endl;
            for(auto A : Interpretation->Associations){
                std::cout << std::string(IndentCount * 2, ' ') << A << std::endl;
            }
        }

        // Type Definitions
        if(Interpretation->TypeDefs.size() > 0){
            std::cout << std::string(IndentCount, ' ') << "TypeDefs: " << std::endl;
            for(auto A : Interpretation->TypeDefs){
                std::cout << std::string(IndentCount * 2, ' ') << A->Identifier << ": s(" << A->DataSize << ")" << ((A->Compound) ? "c" : "") << ((A->Refering.Referenceing) ? "r(" + A->Refering.RelayTypeDef + ")" : "") << std::endl;
            }
        }

        // Object Definitions
        if(Interpretation->ObjectDefs.size() > 0){
            std::cout << std::string(IndentCount, ' ') << "Class/Structs: " << std::endl;
            for(auto A : Interpretation->ObjectDefs){
                std::cout << std::string(IndentCount * 2, ' ') << A->TypeDefinition << "(" << A->Namespace << ")" << std::endl;
                
                // Components
                if(A->Variables.size() > 0){
                    std::cout << std::string(IndentCount * 3, ' ') << "Components: " << std::endl;
                    for(auto C : A->Variables){
                        std::cout << std::string(IndentCount * 4, ' ') << C->Identifier << ": t" << ((C->Pointer) ? "*" : "") << "(" << C->TypeDef << ")" << ((C->Private) ? "P" : "") << ((C->Protected) ? "p" : "") << ((C->Constant) ? "c" : "") << ((C->Static) ? "s" : "") << ((C->Initialiser.size() > 0) ? ("i(" + C->Initialiser + ")") : "") << std::endl;
                    }
                }

                // Functions
                if(A->AssociatedFunctions.size() > 0){
                    std::cout << std::string(IndentCount * 3, ' ') << "Functions: " << std::endl;
                    for(auto C : A->AssociatedFunctions){
                        LogFunctionDefinition(C, 4);
                    }
                }
            }
        }

        // Function Definitions
        if(Interpretation->FunctionDefs.size() > 0){
            std::cout << std::string(IndentCount, ' ') << "Functions: " << std::endl;
            for(auto A : Interpretation->FunctionDefs){
                if(A->Namespace.find("__CLASS__") == A->Namespace.npos){
                    // Not listed as part of classes
                    LogFunctionDefinition(A, 2);
                }
            }
        }

        // Global Variables
        if(Interpretation->Variables.size() > 0){
            bool Global = false;
            for(auto A : Interpretation->Variables){
                if(A->Namespace == "__GLOB__"){
                    if(!Global){
                        Global = true;
                        std::cout << std::string(IndentCount, ' ') << "Global Variables: " << std::endl;
                        std::cout << std::string(IndentCount * 2, ' ') << A->Identifier << ": t" << ((A->Pointer) ? "*" : "") << "(" << A->TypeDef << ")" << ((A->Private) ? "P" : "") << ((A->Protected) ? "p" : "") << ((A->Constant) ? "c" : "") << ((A->Static) ? "s" : "") << ((A->Initialiser.size() > 0) ? ("i(" + A->Initialiser + ")") : "") << std::endl;
                    }
                }
            }
        }
    }

    if(semanticAnalyser->GetFiles().size() <= 0){
        std::cerr << "Failed to analyse!" << std::endl;
        return -1;
    }

    if(CompilerInformation::DebugAll())
        std::cout << "------ End Semantic Analysis ------" << std::endl;

    if(CompilerInformation::DebugAll()){
        std::cout << "Begining Assembly Generation..." << std::endl;
    }

    // Start Generation
    Exporting::AssemblyGenerator* generator = new Exporting::AssemblyGenerator();
    std::string assembly = generator->Generate(semanticAnalyser);

    if(assembly.size() <= 0){
        std::cerr << "Failed to generate assembly!" << std::endl;
        return -1;
    }

    if(CompilerInformation::DebugAll())
        std::cout << "Generated Assembly -> Exporting to " << InputStream::__COMPILATION_CONFIGURATION->OutputFile() << std::endl;

    // Write to file
    {
        InputStream::FileHandler output_file = InputStream::FileHandler(InputStream::__COMPILATION_CONFIGURATION->OutputFile());
        output_file = assembly;
        output_file.Close();
    }

    std::cout << "Compilation Successful! Output: " << InputStream::__COMPILATION_CONFIGURATION->OutputFile() << std::endl;

    return 0;
}