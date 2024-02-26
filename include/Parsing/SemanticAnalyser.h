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
        SemanticVariables::SemanticVariable* RootScope = nullptr;
    public:
        SemanticAnalyser(std::vector<LexicalAnalyser*> AnalysisData);
        ~SemanticAnalyser();
    public:
        SemanticVariables::SemanticVariable* getRootScope() {return RootScope;};
    };
} // namespace Parsing