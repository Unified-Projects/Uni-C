#include <Export/BuilderTools.h>

#include <iostream>

using namespace Exporting;
using namespace Exporting::Helpers;

using namespace Parsing;
using namespace Parsing::SemanticVariables;

extern std::map<std::string, SemanticTypeDefinition> SemanticTypeMap;

StackTrace::StackTrace(){
    stackPointer = 0;
    stackSize = 0;
    stackStart = 0;

    stack = {};
}
StackTrace::~StackTrace(){

}

void StackTrace::Push(RegisterValue scope){
    stack.push_back(scope);
    stackSize++;
    stackPointer = stackSize-1;

    if(stackSize == 1){
        stackStart = 0;
    }
    else{
        stackStart = stackSize-1;
    }

    // Check if stack is too big
    if(stackSize > 0x4 * 0x1000 * 0x1000){
        stack.erase(stack.begin());
        stackSize--;
        stackStart--;
    }
}
RegisterValue StackTrace::Pop(){
    if(stackSize == 0){
        return {"", "", "", "", "", nullptr, false, false, 0, 0, -1};
    }

    stackSize--;
    stackPointer = stackSize+1;

    return stack[stackSize];
}
RegisterValue StackTrace::Pop(int index){
    if(stackSize < index+1 || index < 0){
        return {"", "", "", "", "", nullptr, false, false, 0, 0, -1};
    }

    return stack[index];

}
RegisterValue StackTrace::Peek(){
    if(stackSize == 0){
        return {"", "", "", "", "", nullptr, false, false, 0, 0, -1};
    }

    return stack[stackSize-1];
}
RegisterValue& StackTrace::Peek(int index){
    if(stackSize < index+1 || index < 0){
        static RegisterValue Null = {"", "", "", "", "", nullptr, false, false, 0, 0, -1};
        return Null;
    }

    return stack[stackSize-index];

}

void StackTrace::Clear(){
    stack.clear();
    stackSize = 0;
    stackPointer = 0;
    stackStart = 0;
}

std::string Helpers::GetSymbol(SemanticVariable* Var){
    std::string Symbol = "";

    if(Var->Type() == SemanticTypeVariable){
        Symbol = ((Variable*)Var)->Identifier + "_" + std::to_string(((Variable*)Var)->LocalScope) + "_" + std::to_string(((Variable*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeFunction){
        Symbol = ((Function*)Var)->Identifier + "_" + std::to_string(((Function*)Var)->LocalScope) + "_" + std::to_string(((Function*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeLiteral){
        Symbol = "Value_" + std::to_string(((SemLiteral*)Var)->LocalScope) + "_" + std::to_string(((SemLiteral*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeScope){
        Symbol = "Scope_" + std::to_string(((Scope*)Var)->LocalScope) + "_" + std::to_string(((Scope*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeType){
        Symbol = ((SemTypeDef*)Var)->Identifier + "_" + std::to_string(((SemTypeDef*)Var)->LocalScope) + "_" + std::to_string(((SemTypeDef*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeStatement){
        Symbol = "Statement_" + std::to_string(((SemStatement*)Var)->TypeID) + "_" + std::to_string(((SemStatement*)Var)->LocalScope) + "_" + std::to_string(((SemStatement*)Var)->ScopePosition);
    }
    else if(Var->Type() == SemanticTypeOperation){
        Symbol = "Operation_" + std::to_string(((Operation*)Var)->TypeID) + "_" + std::to_string(((Operation*)Var)->LocalScope) + "_" + std::to_string(((Operation*)Var)->ScopePosition);
    }
    else{
        std::cerr << "Unknown Type: " << Var->Type() << std::endl;
    }

    // Recurse Backwards
    if(Var->Parent != nullptr){
        Symbol = GetSymbol(Var->Parent) + "__" + Symbol;
    }

    return Symbol;
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
            entry->Scope = Root->ScopePosition;
            entry->ScopeLocal = Root->LocalScope;
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
            entry->Scope = Root->ScopePosition;
            entry->ScopeLocal = Root->LocalScope;
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
            entry->Scope = Root->ScopePosition;
            entry->ScopeLocal = Root->LocalScope;
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
            entry->Scope = Root->ScopePosition;
            entry->ScopeLocal = Root->LocalScope;
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
            entry->Scope = Root->ScopePosition;
            entry->ScopeLocal = Root->LocalScope;
            entry->parent = Parent;
            entry->children = {};
            entry->Variables = {};
            entry->TypeDefs = {};
            entry->Functions = {};
            entry->Var = Root;

            ScopeMap[Root] = entry;
        }
        break;
    case SemanticTypeFunctionRef:
        {
            ScopeTreeEntry* entry = new ScopeTreeEntry();
            entry->ScopeIndex = Root->LocalScope;
            entry->Scope = Root->ScopePosition;
            entry->ScopeLocal = Root->LocalScope;
            entry->parent = Parent;
            entry->children = {};
            entry->Variables = {};
            entry->TypeDefs = {};
            entry->Functions = {};
            entry->Var = Root;

            ScopeMap[Root] = entry;
        }
        break;
    case SemanticTypeRegisterRef:
        {
            ScopeTreeEntry* entry = new ScopeTreeEntry();
            entry->ScopeIndex = Root->LocalScope;
            entry->Scope = Root->ScopePosition;
            entry->ScopeLocal = Root->LocalScope;
            entry->parent = Parent;
            entry->children = {};
            entry->Variables = {};
            entry->TypeDefs = {};
            entry->Functions = {};
            entry->Var = Root;

            ScopeMap[Root] = entry;
        }
        break;
    case SemanticTypeLiteral:
        {
            ScopeTreeEntry* entry = new ScopeTreeEntry();
            entry->ScopeIndex = Root->LocalScope;
            entry->Scope = Root->ScopePosition;
            entry->ScopeLocal = Root->LocalScope;
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
            entry->Scope = Root->ScopePosition;
            entry->ScopeLocal = Root->LocalScope;
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
            entry->Scope = Root->LocalScope;
            entry->ScopeLocal = Root->ScopePosition;
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

// TODO : Follow from above local scope position

Parsing::SemanticVariables::SemanticVariable* ScopeTree::FindVariable(std::string& Name, Parsing::SemanticVariables::SemanticVariable* Scope){
    // Find entry to traverse up
    ScopeTreeEntry* entry = ScopeMap[Scope];

    while(entry != nullptr){;
        for(auto var : entry->Variables){
            if(((Variable*)var->Var)->Identifier == Name){
                if(var->Var->ScopePosition > entry->ScopeLocal && entry->Var->ParentScope == entry->Var->LocalScope)
                    continue;

                return var->Var;
            }
        }
        entry = entry->parent;
    }

    return nullptr;
}

Parsing::SemanticVariables::SemanticVariable* ScopeTree::FindFunction(std::string& Name, Parsing::SemanticVariables::SemanticVariable* Scope){
    // Find entry to traverse up
    ScopeTreeEntry* entry = ScopeMap[Scope];

    while(entry != nullptr){;
        for(auto var : entry->Functions){
            if(((Function*)var->Var)->Identifier == Name){
                if(var->Var->ScopePosition > entry->ScopeLocal)
                    continue;

                return var->Var;
            }
        }
        entry = entry->parent;
    }

    return nullptr;
}

RegisterTable::RegisterTable(){
    Clear();
}
RegisterTable::~RegisterTable(){

}
void RegisterTable::Clear(){
    rax = {"rax", "eax", "ax", "al", "", nullptr, false, false, 0, 0, 0};
    rbx = {"rbx", "ebx", "bx", "bl", "", nullptr, false, false, 0, 0, 0};
    rcx = {"rcx", "ecx", "cx", "cl", "", nullptr, false, false, 0, 0, 0};
    rdx = {"rdx", "edx", "dx", "dl", "", nullptr, false, false, 0, 0, 0};
    rsi = {"rsi", "esi", "si", "si", "", nullptr, false, false, 0, 0, 0};
    rdi = {"rdi", "edi", "di", "di", "", nullptr, false, false, 0, 0, 0};
    rbp = {"rbp", "ebp", "bp", "bp", "", nullptr, false, false, 0, 0, 0};
    rsp = {"rsp", "esp", "sp", "sp", "", nullptr, false, false, 0, 0, 0};
    r8  = {"r8",  "r8d", "r8w", "r8b", "", nullptr, false, false, 0, 0, 0};
    r9  = {"r9",  "r9d", "r9w", "r9b", "", nullptr, false, false, 0, 0, 0};
    r10 = {"r10", "r10d", "r10w", "r10b", "", nullptr, false, false, 0, 0, 0};
    r11 = {"r11", "r11d", "r11w", "r11b", "", nullptr, false, false, 0, 0, 0};
    r12 = {"r12", "r12d", "r12w", "r12b", "", nullptr, false, false, 0, 0, 0};
    r13 = {"r13", "r13d", "r13w", "r13b", "", nullptr, false, false, 0, 0, 0};
    r14 = {"r14", "r14d", "r14w", "r14b", "", nullptr, false, false, 0, 0, 0};
    r15 = {"r15", "r15d", "r15w", "r15b", "", nullptr, false, false, 0, 0, 0};
}

std::string RegisterTable::MovReg(StackTrace* stack, std::string r1, std::string r2){
    RegisterValue reg1 = {"", "", "", "", "", nullptr, false, false, 0, 0, -1};
    RegisterValue reg2 = {"", "", "", "", "", nullptr, false, false, 0, 0, -1};

    if(r1 == "rax"){
        reg1 = rax;
    }
    else if(r1 == "rbx"){
        reg1 = rbx;
    }
    else if(r1 == "rcx"){
        reg1 = rcx;
    }
    else if(r1 == "rdx"){
        reg1 = rdx;
    }
    else if(r1 == "rsi"){
        reg1 = rsi;
    }
    else if(r1 == "rdi"){
        reg1 = rdi;
    }
    else if(r1 == "rbp"){
        reg1 = rbp;
    }
    else if(r1 == "rsp"){
        reg1 = rsp;
    }
    else if(r1 == "r8"){
        reg1 = r8;
    }
    else if(r1 == "r9"){
        reg1 = r9;
    }
    else if(r1 == "r10"){
        reg1 = r10;
    }
    else if(r1 == "r11"){
        reg1 = r11;
    }
    else if(r1 == "r12"){
        reg1 = r12;
    }
    else if(r1 == "r13"){
        reg1 = r13;
    }
    else if(r1 == "r14"){
        reg1 = r14;
    }
    else if(r1 == "r15"){
        reg1 = r15;
    }
    else{
        return "; Unknown register error\n";
    }

    if(r2 == "rax"){
        reg2 = rax;
    }
    else if(r2 == "rbx"){
        reg2 = rbx;
    }
    else if(r2 == "rcx"){
        reg2 = rcx;
    }
    else if(r2 == "rdx"){
        reg2 = rdx;
    }
    else if(r2 == "rsi"){
        reg2 = rsi;
    }
    else if(r2 == "rdi"){
        reg2 = rdi;
    }
    else if(r2 == "rbp"){
        reg2 = rbp;
    }
    else if(r2 == "rsp"){
        reg2 = rsp;
    }
    else if(r2 == "r8"){
        reg2 = r8;
    }
    else if(r2 == "r9"){
        reg2 = r9;
    }
    else if(r2 == "r10"){
        reg2 = r10;
    }
    else if(r2 == "r11"){
        reg2 = r11;
    }
    else if(r2 == "r12"){
        reg2 = r12;
    }
    else if(r2 == "r13"){
        reg2 = r13;
    }
    else if(r2 == "r14"){
        reg2 = r14;
    }
    else if(r2 == "r15"){
        reg2 = r15;
    }
    else{
        return "; Unknown register error\n";
    }

    if(reg1.Var){
        this->PushToStack(stack, reg1.Register);
        this->Allocations.remove(&reg1);
    }

    this->Allocations.remove(&reg2);
    this->Allocations.push_back(&reg1);
    
    reg1.Update(&reg2);
    
    return "mov " + reg1.Register + ", " + reg2.Register + "\n";
}

std::string RegisterTable::PullFromStack(StackTrace* stack, std::string registerName, int at){
    auto reg = stack->Peek(at);
    if(!at){
        stack->Pop();
    }
    else{
        stack->Peek(at).Var = nullptr;
    }
    if(reg.TypeID == -1){
        return "; Stack is empty\n";
    }
    if(registerName == "rax"){
        rax.Update(&reg);
        return "pop rax\n";
    }
    else if(registerName == "rbx"){
        rbx.Update(&reg);
        return "pop rbx\n";
    }
    else if(registerName == "rcx"){
        rcx.Update(&reg);
        return "pop rcx\n";
    }
    else if(registerName == "rdx"){
        rdx.Update(&reg);
        return "pop rdx\n";
    }
    else if(registerName == "rsi"){
        rsi.Update(&reg);
        return "pop rsi\n";
    }
    else if(registerName == "rdi"){
        rdi.Update(&reg);
        return "pop rdi\n";
    }
    else if(registerName == "rbp"){
        rbp.Update(&reg);
        return "pop rbp\n";
    }
    else if(registerName == "rsp"){
        rsp.Update(&reg);
        return "pop rsp\n";
    }
    else if(registerName == "r8"){
        r8.Update(&reg);
        return "pop r8\n";
    }
    else if(registerName == "r9"){
        r9.Update(&reg);
        return "pop r9\n";
    }
    else if(registerName == "r10"){
        r10.Update(&reg);
        return "pop r10\n";
    }
    else if(registerName == "r11"){
        r11.Update(&reg);
        return "pop r11\n";
    }
    else if(registerName == "r12"){
        r12.Update(&reg);
        return "pop r12\n";
    }
    else if(registerName == "r13"){
        r13.Update(&reg);
        return "pop r13\n";
    }
    else if(registerName == "r14"){
        r14.Update(&reg);
        return "pop r14\n";
    }
    else if(registerName == "r15"){
        r15.Update(&reg);
        return "pop r15\n";
    }
    
    return "; Unknown register error\n";
}

std::string RegisterTable::PushToStack(StackTrace* stack, std::string registerName){
    if(registerName == "rax"){
        stack->Push(rax);
        return "push rax\n";
    }
    else if(registerName == "rbx"){
        stack->Push(rbx);
        return "push rbx\n";
    }
    else if(registerName == "rcx"){
        stack->Push(rcx);
        return "push rcx\n";
    }
    else if(registerName == "rdx"){
        stack->Push(rdx);
        return "push rdx\n";
    }
    else if(registerName == "rsi"){
        stack->Push(rsi);
        return "push rsi\n";
    }
    else if(registerName == "rdi"){
        stack->Push(rdi);
        return "push rdi\n";
    }
    else if(registerName == "rbp"){
        stack->Push(rbp);
        return "push rbp\n";
    }
    else if(registerName == "rsp"){
        stack->Push(rsp);
        return "push rsp\n";
    }
    else if(registerName == "r8"){
        stack->Push(r8);
        return "push r8\n";
    }
    else if(registerName == "r9"){
        stack->Push(r9);
        return "push r9\n";
    }
    else if(registerName == "r10"){
        stack->Push(r10);
        return "push r10\n";
    }
    else if(registerName == "r11"){
        stack->Push(r11);
        return "push r11\n";
    }
    else if(registerName == "r12"){
        stack->Push(r12);
        return "push r12\n";
    }
    else if(registerName == "r13"){
        stack->Push(r13);
        return "push r13\n";
    }
    else if(registerName == "r14"){
        stack->Push(r14);
        return "push r14\n";
    }
    else if(registerName == "r15"){
        stack->Push(r15);
        return "push r15\n";
    }
    
    return "; Unknown register error\n";
}

void RegisterTable::NewSave(StackTrace* stack){
    SavePoint.push_back(stack->Size());
}

std::string RegisterTable::CorrectStack(StackTrace* stack, int IndentIndex){
    if(SavePoint.size() == 0){
        return "; No save point\n";
    }

    int size = SavePoint[SavePoint.size()-1];
    SavePoint.pop_back();

    int Removed = 0;

    while(stack->Size() > size){
        stack->Pop();
        Removed++;
    }

    if(!Removed){
        return ";; No stack correction needed\n";
    }

    return "; Stack correction\n" + std::string(IndentIndex*4, ' ') + "add rsp, " + std::to_string((Removed) * 8) + "\n";

}

std::string RegisterTable::CorrectStackNoSaveDec(StackTrace* stack, int IndentIndex){
    if(SavePoint.size() == 0){
        return "; No save point\n";
    }

    int size = SavePoint[SavePoint.size()-1];

    int Removed = 0;

    while(stack->Size() > size){
        stack->Pop();
        Removed++;
    }

    if(!Removed){
        return ";; No stack correction needed\n";
    }

    return "; Stack correction\n" + std::string(IndentIndex*4, ' ') + "add rsp, " + std::to_string((Removed) * 8) + "\n";
}

RegisterValue* RegisterTable::GetFreeReg(StackTrace* stack, std::string& ReturnString, int IndentIndex){
    if(r8.Var == nullptr){
        return &r8;
    }
    else if(r9.Var == nullptr){
        return &r9;
    }
    else if(r10.Var == nullptr){
        return &r10;
    }
    else if(r11.Var == nullptr){
        return &r11;
    }
    else if(r12.Var == nullptr){
        return &r12;
    }
    else if(r13.Var == nullptr){
        return &r13;
    }
    else if(r14.Var == nullptr){
        return &r14;
    }
    else if(r15.Var == nullptr){
        return &r15;
    }
    else if(rdi.Var == nullptr){
        return &rdi;
    }
    else if(rsi.Var == nullptr){
        return &rsi;
    }
    else if(rdx.Var == nullptr){
        return &rdx;
    }
    else if(rcx.Var == nullptr){
        return &rcx;
    }
    else if(rbx.Var == nullptr){
        return &rbx;
    }
    else if(rax.Var == nullptr){
        return &rax;
    }

    // Load last used to stack
    RegisterValue* FirstAllocated = Allocations.front();
    Allocations.pop_front();

    ReturnString += std::string(IndentIndex*4, ' ') + this->PushToStack(stack, FirstAllocated->Register);

    return FirstAllocated;
}
RegisterValue* RegisterTable::GetVariable(StackTrace* stack, Parsing::SemanticVariables::SemanticVariable* Var, std::string& ReturnString, int IndentIndex){
    // Is it a register ref
    if(Var->Type() == SemanticTypeRegisterRef){
        // We need its specific register
        auto Ref = (RegisterRef*)Var;
        RegisterValue* Reg = nullptr;

        if(Ref->Register == "rax"){
            Reg = &rax;
        }
        else if(Ref->Register == "rbx"){
            Reg = &rbx;
        }
        else if(Ref->Register == "rcx"){
            Reg = &rcx;
        }
        else if(Ref->Register == "rdx"){
            Reg = &rdx;
        }
        else if(Ref->Register == "rsi"){
            Reg = &rsi;
        }
        else if(Ref->Register == "rdi"){
            Reg = &rdi;
        }
        else if(Ref->Register == "rbp"){
            Reg = &rbp;
        }
        else if(Ref->Register == "rsp"){
            Reg = &rsp;
        }
        else if(Ref->Register == "r8"){
            Reg = &r8;
        }
        else if(Ref->Register == "r9"){
            Reg = &r9;
        }
        else if(Ref->Register == "r10"){
            Reg = &r10;
        }
        else if(Ref->Register == "r11"){
            Reg = &r11;
        }
        else if(Ref->Register == "r12"){
            Reg = &r12;
        }
        else if(Ref->Register == "r13"){
            Reg = &r13;
        }
        else if(Ref->Register == "r14"){
            Reg = &r14;
        }
        else if(Ref->Register == "r15"){
            Reg = &r15;
        }
        else{
            return nullptr;
        }

        if(Reg->Var == Var){
            this->Allocations.remove(Reg);
            this->Allocations.push_back(Reg);
            return Reg;
        }
        else{
            if(Reg->Var != nullptr){
                ReturnString += std::string(IndentIndex*4, ' ') + this->PushToStack(stack, Reg->Register);
                Allocations.remove(Reg);
            }

            // Is this in the stack
            // Is it in the stack
            for(auto i = 0; i < stack->Size(); i++){
                if(stack->Peek(i).Var == Var){
                    ReturnString += std::string(IndentIndex*4, ' ') + "sub rsp, " + std::to_string((stack->Size()-i) * 8) + "\n";
                    stack->Peek(i).Var = nullptr;
                    ReturnString += std::string(IndentIndex*4, ' ') + "pop " + Reg->Register + "\n";
                    ReturnString += std::string(IndentIndex*4, ' ') + "add rsp, " + std::to_string((stack->Size()-i-1) * 8) + "\n";

                    Reg->Var = Var;
                    Reg->Size = Ref->Size;
                    Reg->TypeID = 0;

                    Allocations.push_back(Reg);

                    return Reg;
                }
            }

            Reg->Var = Var;
            Reg->Size = Ref->Size;
            Reg->TypeID = 0;

            Allocations.push_back(Reg);

            return Reg;
        }
    }

    // Already in a register?
    if(r8.Var == Var){
        this->Allocations.remove(&r8);
        this->Allocations.push_back(&r8);
        return &r8;
    }
    else if(r9.Var == Var){
        this->Allocations.remove(&r9);
        this->Allocations.push_back(&r9);
        return &r9;
    }
    else if(r10.Var == Var){
        this->Allocations.remove(&r10);
        this->Allocations.push_back(&r10);
        return &r10;
    }
    else if(r11.Var == Var){
        this->Allocations.remove(&r11);
        this->Allocations.push_back(&r11);
        return &r11;
    }
    else if(r12.Var == Var){
        this->Allocations.remove(&r12);
        this->Allocations.push_back(&r12);
        return &r12;
    }
    else if(r13.Var == Var){
        this->Allocations.remove(&r13);
        this->Allocations.push_back(&r13);
        return &r13;
    }
    else if(r14.Var == Var){
        this->Allocations.remove(&r14);
        this->Allocations.push_back(&r14);
        return &r14;
    }
    else if(r15.Var == Var){
        this->Allocations.remove(&r15);
        this->Allocations.push_back(&r15);
        return &r15;
    }
    else if(rdi.Var == Var){
        this->Allocations.remove(&rdi);
        this->Allocations.push_back(&rdi);
        return &rdi;
    }
    else if(rsi.Var == Var){
        this->Allocations.remove(&rsi);
        this->Allocations.push_back(&rsi);
        return &rsi;
    }
    else if(rdx.Var == Var){
        this->Allocations.remove(&rdx);
        this->Allocations.push_back(&rdx);
        return &rdx;
    }
    else if(rcx.Var == Var){
        this->Allocations.remove(&rcx);
        this->Allocations.push_back(&rcx);
        return &rcx;
    }
    else if(rbx.Var == Var){
        this->Allocations.remove(&rbx);
        this->Allocations.push_back(&rbx);
        return &rbx;
    }
    else if(rax.Var == Var){
        this->Allocations.remove(&rax);
        this->Allocations.push_back(&rax);
        return &rax;
    }

    // Get a freed register
    auto FreeReg = GetFreeReg(stack, ReturnString, IndentIndex);

    if(Var->Type() == SemanticTypeFunctionRef){
        if(FreeReg->Var != nullptr){
            ReturnString += std::string(IndentIndex*4, ' ') + this->PushToStack(stack, FreeReg->Register);
            Allocations.remove(FreeReg);
        }

        // Is this in the stack
        // Is it in the stack
        for(auto i = 0; i < stack->Size(); i++){
            if(stack->Peek(i).Var == Var){
                ReturnString += std::string(IndentIndex*4, ' ') + "sub rsp, " + std::to_string((stack->Size()-i) * 8) + "\n";
                stack->Peek(i).Var = nullptr;
                ReturnString += std::string(IndentIndex*4, ' ') + "pop " + FreeReg->Register + "\n";
                ReturnString += std::string(IndentIndex*4, ' ') + "add rsp, " + std::to_string((stack->Size()-i-1) * 8) + "\n";

                FreeReg->Var = Var;
                FreeReg->TypeID = 0;
                FreeReg->Size = ((Operation*)Var)->EndSize;

                Allocations.push_back(FreeReg);

                return FreeReg;
            }
        }

        FreeReg->Var = Var;
        FreeReg->TypeID = 0;
        FreeReg->Size = ((Operation*)Var)->EndSize;

        Allocations.push_back(FreeReg);

        return FreeReg;
    }

    // Is it in the stack
    for(auto i = 0; i < stack->Size(); i++){
        if(stack->Peek(i).Var == Var){
            ReturnString += std::string(IndentIndex*4, ' ') + "sub rsp, " + std::to_string((stack->Size()-i) * 8) + "\n";
            ReturnString += std::string(IndentIndex*4, ' ') + this->PullFromStack(stack, FreeReg->Register, i);
            ReturnString += std::string(IndentIndex*4, ' ') + "add rsp, " + std::to_string((stack->Size()-i-1) * 8) + "\n";
            auto Reg = stack->Peek(i);
            FreeReg->Update(&Reg);
            Allocations.push_back(FreeReg);
            return FreeReg;
        }
    }

    // Nope so we clearly need to load it
    Allocations.push_back(FreeReg);
    if (Var->Type() == SemanticTypeLiteral){
        ReturnString += std::string(IndentIndex*4, ' ') + FreeReg->set((SemLiteral*)Var);
    }
    else{
        ReturnString += std::string(IndentIndex*4, ' ') + FreeReg->set((Variable*)Var, false);
    }

    return FreeReg;
}

void RegisterTable::ReleaseReg(RegisterValue* reg){
    this->Allocations.remove(reg);
    reg->Var = nullptr;
    reg->IsPointer = false;
    reg->IsValue = false;
    reg->Size = 0;
    reg->Value = 0;
    reg->TypeID = -1;
}

std::string RegisterValue::get(){
    switch (this->Size)
    {
    case 8:
        return this->Register;
    case 4:
        return this->Register1;
    case 2:
        return this->Register2;
    case 1:
        return this->Register3;

    default:
        return "; Unknown register error\n";
    }
}

std::string RegisterValue::set(Parsing::SemanticVariables::Variable* variable, bool isPointer){
    this->Var = variable;
    this->IsPointer = isPointer;
    this->IsValue = false;
    this->TypeID = variable->TypeID;
    
    // Find size
    if(!isPointer){
        for(auto type : SemanticTypeMap){
            if(type.second.TypeID == variable->TypeID){
                this->Size = type.second.Size;
            }
        }
    }
    else{
        this->Size = 8;
    }
    this->Value = 0;

    // What to return
    return (isPointer ? "mov " + this->Register + ", " : "mov " + this->Register + ", [") + GetSymbol(variable) + (isPointer ? "" : "]") + "\n";
}
std::string RegisterValue::save(){
    if(this->Var == nullptr){
        return "; No variable to save\n";
    }
    if(this->IsPointer){
        return "; No need to save pointer\n";
    }
    else{
        if(this->IsValue){
            return "";
        }

        if(Var->Type() == SemanticTypeVariable){
            auto var = (Variable*)Var;
            // if(var->IsGlobal){
            //     return "; No need to save global\n";
            // }

            return "mov [" + GetSymbol(this->Var) + "], " + this->Register + "\n";
        }
        else if(Var->Type() == SemanticTypeRegisterRef){
            return "";
        }
        else{
            return "; Unknown type error\n";
        }
    }

    return "; Unknown error\n";
}
std::string RegisterValue::set(Parsing::SemanticVariables::SemLiteral* variable){
    this->Var = variable;
    this->IsPointer = false;
    this->IsValue = true;
    this->TypeID = variable->TypeID;
    
    if(variable->TypeID == SemanticTypeMap["int"].TypeID || variable->TypeID == SemanticTypeMap["uint"].TypeID){
        this->Value = (uint32_t)std::atoi(variable->Value.c_str());
        return this->set((uint32_t)this->Value);
    }
    else if(variable->TypeID == SemanticTypeMap["short"].TypeID || variable->TypeID == SemanticTypeMap["ushort"].TypeID){
        this->Value = (uint16_t)std::atoi(variable->Value.c_str());
        return this->set((uint16_t)this->Value);
    }
    else if(variable->TypeID == SemanticTypeMap["long"].TypeID || variable->TypeID == SemanticTypeMap["ulong"].TypeID){
        this->Value = (uint64_t)std::atoll(variable->Value.c_str());
        return this->set((uint64_t)this->Value);
    }
    else if(variable->TypeID == SemanticTypeMap["float"].TypeID){
        this->Value = (uint32_t)std::atof(variable->Value.c_str());
        return this->set((uint32_t)this->Value);
    }
    else if(variable->TypeID == SemanticTypeMap["double"].TypeID){
        this->Value = (uint64_t)std::atof(variable->Value.c_str());
        return this->set((uint64_t)this->Value);
    }
    else if(variable->TypeID == SemanticTypeMap["char"].TypeID || variable->TypeID == SemanticTypeMap["uchar"].TypeID){
        this->Value = (uint8_t)variable->Value[0];
        return this->set((uint8_t)this->Value);
    }
    else if(variable->TypeID == SemanticTypeMap["bool"].TypeID){
        this->Value = (uint8_t)variable->Value[0];
        return this->set((uint8_t)this->Value);
    }
    else if(variable->TypeID == SemanticTypeMap["void"].TypeID){
        return "; Cannot set void\n";
    }
    else if(variable->TypeID == SemanticTypeMap["string"].TypeID){
        // Would store size in bytes as string length
        return "; Cannot set string\n";
    }
    
    std::cerr << "Unknown type: " << variable->TypeID << std::endl;
    return "; Unknown type error"; // TODO CUstoms
}

std::string RegisterValue::set(uint64_t value){
    this->IsPointer = false;
    this->IsValue = true;
    this->Size = 8;
    this->Value = value;
    
    return "mov " + this->Register + ", " + std::to_string(value) + "\n";
}
std::string RegisterValue::set(uint32_t value){
    this->IsPointer = false;
    this->IsValue = true;
    this->Size = 4;
    this->Value = value;
    
    return "mov " + this->Register1 + ", " + std::to_string(value) + "\n";
}
std::string RegisterValue::set(uint16_t value){
    this->IsPointer = false;
    this->IsValue = true;
    this->Size = 2;
    this->Value = value;

    return "mov " + this->Register2 + ", " + std::to_string(value) + "\n";
}
std::string RegisterValue::set(uint8_t value){
    this->IsPointer = false;
    this->IsValue = true;
    this->Size = 1;
    this->Value = value;

    return "mov " + this->Register3 + ", " + std::to_string(value) + "\n";
}