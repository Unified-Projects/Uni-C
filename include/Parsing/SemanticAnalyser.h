#pragma once    

#include <string>
#include <Parsing/LexicalAnalyser.h>
#include <Parsing/SemanticVariables.h>

namespace Parsing
{
    class SemanticAnalyser
    {
    protected:
        std::vector<LexicalAnalyser*> AnalysisData = {};
        std::vector<SemanticVariables::SemanticisedFile*> LoadedFiles = {};
    public:
        SemanticAnalyser(std::vector<LexicalAnalyser*> AnalysisData);
        ~SemanticAnalyser();

        std::vector<SemanticVariables::SemanticisedFile*> GetFiles() {return this->LoadedFiles;}
    };
} // namespace Parsing