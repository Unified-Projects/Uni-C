#pragma once

#include <Parsing/SemanticVariables.h>

#include <list>

namespace Exporting
{
    namespace Helpers
    {
        std::string GetSymbol(Parsing::SemanticVariables::SemanticVariable* Var);

        class StackTrace;

        struct RegisterValue{
            std::string Register = "";
            std::string Register1 = "";
            std::string Register2 = "";
            std::string Register3 = "";
            std::string Register4 = "";
            Parsing::SemanticVariables::SemanticVariable* Var = nullptr;
            bool IsPointer = false;
            bool IsValue = false;
            int Size = 0;
            uint64_t Value = 0;
            int TypeID = -1;

            void Update(RegisterValue* value){
                Var = value->Var;
                IsPointer = value->IsPointer;
                IsValue = value->IsValue;
                Size = value->Size;
                Value = value->Value;
                TypeID = value->TypeID;
            }

            std::string get();

            std::string set(Parsing::SemanticVariables::Variable* variable, bool isPointer);
            std::string set(Parsing::SemanticVariables::SemLiteral* variable);
            std::string save();

            std::string set(uint64_t value);
            std::string set(uint32_t value);
            std::string set(uint16_t value);
            std::string set(uint8_t value);
        };

        class StackTrace{
        protected:
            int stackPointer = 0;
            int stackSize = 0;
            int stackStart = 0;

            std::vector<RegisterValue> stack = {};
        public:
            StackTrace();
            ~StackTrace();
        
        public:
            void Push(RegisterValue scope);
            RegisterValue Pop();
            RegisterValue Pop(int index);
            RegisterValue Peek();
            RegisterValue Peek(int index);

            void Clear();

            int Size() {return stackSize;};
            int Pointer() {return stackPointer;};
        };

        struct ScopeTreeEntry{
            int ScopeIndex = 0;
            ScopeTreeEntry* parent = nullptr;
            Parsing::SemanticVariables::SemanticVariable* Var = nullptr;
            std::vector<ScopeTreeEntry*> children = {};
            std::vector<ScopeTreeEntry*> Variables = {};
            std::vector<ScopeTreeEntry*> TypeDefs = {};
            std::vector<ScopeTreeEntry*> Functions = {};
        };

        class ScopeTree{
        protected:
            ScopeTreeEntry* Root = nullptr;
            std::map<Parsing::SemanticVariables::SemanticVariable*, ScopeTreeEntry*> ScopeMap = {};
        public:
            ScopeTree();
            ~ScopeTree();

            void Generate(Parsing::SemanticVariables::SemanticVariable* RootScope);

        public:
            Parsing::SemanticVariables::SemanticVariable* FindVariable(std::string& Name, Parsing::SemanticVariables::SemanticVariable* Scope);

        protected:
            void GenerateTree(Parsing::SemanticVariables::SemanticVariable* Scope, ScopeTreeEntry* Parent);
        };
        
        class RegisterTable{
        public:
            RegisterValue rax = {};
            RegisterValue rbx = {};
            RegisterValue rcx = {};
            RegisterValue rdx = {};
            RegisterValue rsi = {};
            RegisterValue rdi = {};
            RegisterValue rbp = {};
            RegisterValue rsp = {};
            RegisterValue r8 = {};
            RegisterValue r9 = {};
            RegisterValue r10 = {};
            RegisterValue r11 = {};
            RegisterValue r12 = {};
            RegisterValue r13 = {};
            RegisterValue r14 = {};
            RegisterValue r15 = {};

            std::vector<int> SavePoint = {};
            std::list<RegisterValue*> Allocations = {};
        public:
            RegisterTable();
            ~RegisterTable();
            
        public:
            std::string PullFromStack(StackTrace* stack, std::string registerName);
            std::string PushToStack(StackTrace* stack, std::string registerName);

            std::string MovReg(StackTrace* stack, std::string register1, std::string register2);

            void NewSave(StackTrace* stack);
            std::string CorrectStack(StackTrace* stack, int IndentIndex);
            std::string CorrectStackNoSaveDec(StackTrace* stack, int IndentIndex);
            RegisterValue* GetFreeReg(StackTrace* stack, std::string& ReturnString, int IndentIndex);
            void ReleaseReg(RegisterValue* reg);
            RegisterValue* GetVariable(StackTrace* stack, Parsing::SemanticVariables::SemanticVariable* Var, std::string& ReturnString, int IndentIndex);

            void Clear();
        };
    } // namespace SemanticHelpers
} // namespace Exporting
