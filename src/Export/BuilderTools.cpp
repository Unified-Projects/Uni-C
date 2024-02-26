#include <Export/BuilderTools.h>

using namespace Exporting;
using namespace Exporting::Helpers;

using namespace Parsing;
using namespace Parsing::SemanticVariables;

StackTrace::StackTrace(){
    stackPointer = 0;
    stackSize = 0;
    stackStart = 0;

    stack = {};
}
StackTrace::~StackTrace(){

}

ScopeTree::ScopeTree(){
    Root = nullptr;
}
ScopeTree::~ScopeTree(){
    delete Root;
}

void ScopeTree::Generate(SemanticVariable* RootScope){
    if(RootScope->Type() != SemanticTypeScope){
        return;
    }
    Root = new ScopeTreeEntry();
    Root->ScopeIndex = RootScope->LocalScope;
    Root->parent = nullptr;
    Root->children = {};
    Root->Variables = {};
    Root->TypeDefs = {};
    Root->Functions = {};
    Root->Var = RootScope;

    ScopeMap[RootScope] = Root;

    for(auto child : ((Scope*)RootScope)->Block){
        GenerateTree(child, Root);
    }
}

void ScopeTree::GenerateTree(SemanticVariable* Root, ScopeTreeEntry* Parent){
    if(Root == nullptr) return;
    switch (Root->Type())
    {
    case SemanticTypeScope:
        {
            ScopeTreeEntry* entry = new ScopeTreeEntry();
            entry->ScopeIndex = Root->LocalScope;
            entry->parent = Parent;
            entry->children = {};
            entry->Variables = {};
            entry->TypeDefs = {};
            entry->Functions = {};
            entry->Var = Root;

            ScopeMap[Root] = entry;

            Parent->children.push_back(entry);

            for(auto child : ((Scope*)Root)->Block){
                GenerateTree(child, Parent);
            }
        }
        break;
    case SemanticTypeFunction:
        {
            ScopeTreeEntry* entry = new ScopeTreeEntry();
            entry->ScopeIndex = Root->LocalScope;
            entry->parent = Parent;
            entry->children = {};
            entry->Variables = {};
            entry->TypeDefs = {};
            entry->Functions = {};
            entry->Var = Root;

            ScopeMap[Root] = entry;

            // Load to Parent
            Parent->Functions.push_back(entry);

            for(auto child : ((Function*)Root)->Block){
                GenerateTree(child, Parent);
            }

            for(auto child : ((Function*)Root)->Parameters){
                GenerateTree(child, Parent);
            }
        }
        break;
    case SemanticTypeVariable:
        {
            ScopeTreeEntry* entry = new ScopeTreeEntry();
            entry->ScopeIndex = Root->LocalScope;
            entry->parent = Parent;
            entry->children = {};
            entry->Variables = {};
            entry->TypeDefs = {};
            entry->Functions = {};
            entry->Var = Root;

            ScopeMap[Root] = entry;

            // Load to Parent
            Parent->Variables.push_back(entry);
        }
        break;
    case SemanticTypeType:
        {
            ScopeTreeEntry* entry = new ScopeTreeEntry();
            entry->ScopeIndex = Root->LocalScope;
            entry->parent = Parent;
            entry->children = {};
            entry->Variables = {};
            entry->TypeDefs = {};
            entry->Functions = {};
            entry->Var = Root;

            ScopeMap[Root] = entry;

            // Load to Parent
            Parent->TypeDefs.push_back(entry);

            for(auto child : ((SemTypeDef*)Root)->Attributes){
                GenerateTree(child, Parent);
            }
            for(auto child : ((SemTypeDef*)Root)->Methods){
                GenerateTree(child, Parent);
            }
        }
    case SemanticTypeVariableRef:
        {
            ScopeTreeEntry* entry = new ScopeTreeEntry();
            entry->ScopeIndex = Root->LocalScope;
            entry->parent = Parent;
            entry->children = {};
            entry->Variables = {};
            entry->TypeDefs = {};
            entry->Functions = {};
            entry->Var = Root;

            ScopeMap[Root] = entry;

            // Load to Parent
            Parent->Variables.push_back(entry);
        }
        break;
    case SemanticTypeLiteral:
        {
            ScopeTreeEntry* entry = new ScopeTreeEntry();
            entry->ScopeIndex = Root->LocalScope;
            entry->parent = Parent;
            entry->children = {};
            entry->Variables = {};
            entry->TypeDefs = {};
            entry->Functions = {};
            entry->Var = Root;

            ScopeMap[Root] = entry;

            // Load to Parent
            Parent->Variables.push_back(entry);
        }
        break;
    case SemanticTypeStatement:
        {
            ScopeTreeEntry* entry = new ScopeTreeEntry();
            entry->ScopeIndex = Root->LocalScope;
            entry->parent = Parent;
            entry->children = {};
            entry->Variables = {};
            entry->TypeDefs = {};
            entry->Functions = {};
            entry->Var = Root;

            ScopeMap[Root] = entry;

            // Load to Parent
            Parent->Variables.push_back(entry);

            for(auto child : ((SemStatement*)Root)->Parameters){
                GenerateTree(child, Parent);
            }
            for(auto child : ((SemStatement*)Root)->Block){
                GenerateTree(child, Parent);
            }
        }
        break;
    case SemanticTypeOperation:
        {
            ScopeTreeEntry* entry = new ScopeTreeEntry();
            entry->ScopeIndex = Root->LocalScope;
            entry->parent = Parent;
            entry->children = {};
            entry->Variables = {};
            entry->TypeDefs = {};
            entry->Functions = {};
            entry->Var = Root;

            ScopeMap[Root] = entry;

            // Load to Parent
            Parent->Variables.push_back(entry);

            for(auto child : ((Operation*)Root)->Parameters){
                GenerateTree(child, Parent);
            }
        }
        break;

    default:
        break;
    }
}

#include <iostream>

Parsing::SemanticVariables::SemanticVariable* ScopeTree::FindVariable(std::string& Name, Parsing::SemanticVariables::SemanticVariable* Scope){
    // Find entry to traverse up
    ScopeTreeEntry* entry = ScopeMap[Scope];

    while(entry != nullptr){;
        for(auto var : entry->Variables){
            if(((Variable*)var->Var)->Identifier == Name){
                if(var->Var->ScopePosition > Scope->ScopePosition && var->Var->LocalScope == Scope->LocalScope)
                    return nullptr;



                return var->Var;
            }
        }
        entry = entry->parent;
    }

    return nullptr;
}

RegisterTable::RegisterTable(){
    rax = 0;
    rbx = 0;
    rcx = 0;
    rdx = 0;
    rsi = 0;
    rdi = 0;
    rbp = 0;
    rsp = 0;
    r8 = 0;
    r9 = 0;
    r10 = 0;
    r11 = 0;
    r12 = 0;
    r13 = 0;
    r14 = 0;
    r15 = 0;
}
RegisterTable::~RegisterTable(){

}
void RegisterTable::Clear(){
    rax = 0;
    rbx = 0;
    rcx = 0;
    rdx = 0;
    rsi = 0;
    rdi = 0;
    rbp = 0;
    rsp = 0;
    r8 = 0;
    r9 = 0;
    r10 = 0;
    r11 = 0;
    r12 = 0;
    r13 = 0;
    r14 = 0;
    r15 = 0;
}