#pragma once

#include <Parsing/SemanticVariables.h>

namespace Exporting
{
    namespace Helpers
    {
        class StackTrace{
        protected:
            int stackPointer = 0;
            int stackSize = 0;
            int stackStart = 0;

            std::vector<Parsing::SemanticVariables::SemanticVariable*> stack = {};
        public:
            StackTrace();
            ~StackTrace();
        
        public:
            void Push(Parsing::SemanticVariables::SemanticVariable* scope);
            Parsing::SemanticVariables::SemanticVariable* Pop();
            Parsing::SemanticVariables::SemanticVariable* Peek();

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
        protected:
            uint64_t rax = 0;
            uint64_t rbx = 0;
            uint64_t rcx = 0;
            uint64_t rdx = 0;
            uint64_t rsi = 0;
            uint64_t rdi = 0;
            uint64_t rbp = 0;
            uint64_t rsp = 0;
            uint64_t r8 = 0;
            uint64_t r9 = 0;
            uint64_t r10 = 0;
            uint64_t r11 = 0;
            uint64_t r12 = 0;
            uint64_t r13 = 0;
            uint64_t r14 = 0;
            uint64_t r15 = 0;
        public:
            RegisterTable();
            ~RegisterTable();
        
        public:
            uint64_t getRAX() {return rax;};
            uint64_t getRBX() {return rbx;};
            uint64_t getRCX() {return rcx;};
            uint64_t getRDX() {return rdx;};
            uint64_t getRSI() {return rsi;};
            uint64_t getRDI() {return rdi;};
            uint64_t getRBP() {return rbp;};
            uint64_t getRSP() {return rsp;};
            uint64_t getR8() {return r8;};
            uint64_t getR9() {return r9;};
            uint64_t getR10() {return r10;};
            uint64_t getR11() {return r11;};
            uint64_t getR12() {return r12;};
            uint64_t getR13() {return r13;};
            uint64_t getR14() {return r14;};
            uint64_t getR15() {return r15;};

            void setRAX(uint64_t value) {rax = value;};
            void setRBX(uint64_t value) {rbx = value;};
            void setRCX(uint64_t value) {rcx = value;};
            void setRDX(uint64_t value) {rdx = value;};
            void setRSI(uint64_t value) {rsi = value;};
            void setRDI(uint64_t value) {rdi = value;};
            void setRBP(uint64_t value) {rbp = value;};
            void setRSP(uint64_t value) {rsp = value;};
            void setR8(uint64_t value) {r8 = value;};
            void setR9(uint64_t value) {r9 = value;};
            void setR10(uint64_t value) {r10 = value;};
            void setR11(uint64_t value) {r11 = value;};
            void setR12(uint64_t value) {r12 = value;};
            void setR13(uint64_t value) {r13 = value;};
            void setR14(uint64_t value) {r14 = value;};
            void setR15(uint64_t value) {r15 = value;};
            
        public:
            void Clear();
        };
    } // namespace SemanticHelpers
} // namespace Exporting
