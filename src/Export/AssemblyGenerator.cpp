#include <Export/AssemblyGenerator.h>
#include <CompilerInformation.h>

#include <algorithm>
#include <stack>
#include <regex>

#include <Export/InternalFunctions.h>

using namespace Exporting;
using namespace Exporting::Helpers;
using namespace Parsing;
using namespace Parsing::SemanticVariables;

#include <iostream>

// Registers
// R8-15 are Generic and usable
// RAX - Return value
// RSP - Stack reserve
// RBX-RDX RSI,RDI,RBP - Argument (WinAPI uses RCX, RDX, R8, R9)

// XMM0-XMM15 - FLOAT

extern std::map<std::string, SemanticTypeDefDeclaration*> StandardisedTypes;
extern std::map<SemanticTypeDefDeclaration*, SemanticObjectDefinition*> StandardObjectTypes;

extern std::string GenerateID();

std::vector<std::string> ArgumentRegisters {
    "rbx",
    "rcx",
    "rdx",
    "rsi",
    "rdi",
    "rbp",
};

std::string AssemblyGenerator::InterpretOperation(SemanticisedFile* File, SemanticFunctionDeclaration* Function, SemanticOperation* Operation, SemanticBlock* ParentBlock){
    std::string Assembly = "";

    // Two registers are needed
    std::string OperationID = GenerateID();
    std::string Temp1Symbol = GenerateID();
    std::string Temp2Symbol = GenerateID();
    RegisterDefinition* Register1 = this->Stack->GetTempRegister(Temp1Symbol, Assembly, OperationID);
    RegisterDefinition* Register2 = this->Stack->GetTempRegister(Temp2Symbol, Assembly, OperationID);

    std::stack<std::string> Types = {};

    // Interpret operations
    for(auto Oper : Operation->Operations){
        if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VALUE){
            if(Register1->SetVal(Oper) == ""){
                std::cerr << "ASM::" << __LINE__ << " (" << Oper->TypeDef << ") cannot be used in operation" << std::endl;
                return "";
            }
            Assembly += Register1->SetVal(Oper);
            Assembly += "    push " + Register1->Get(8) + "\n";
            Stack->Push();
            Types.push(Oper->TypeDef);
        }
        else if (Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VARIABLE){
            // NOT STACK BASED: You may ask why, but if you ran it and saw the performance you would make the decision to go memory inefficient.
            // auto Var = Stack->FindSymbol(Oper->Value, Oper->TypeDef, Assembly);
            // Assembly += "    push " + Var->Get(8) + "\n";
            // Types.push(Oper->TypeDef);

            // Get the variables value
            Assembly += "    mov " + Register1->GetBitOp(StandardisedTypes[Oper->TypeDef]->DataSize) + " " + Register1->Get(StandardisedTypes[Oper->TypeDef]->DataSize) + ", [" + Oper->Value + "]\n"; // TODO Create a GLOBAL TYPE-MAP
            Assembly += "    push " + Register1->Get(8) + "\n"; // Push to stack
            Stack->Push();
            Types.push(Oper->TypeDef);
        }
        else if (Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_VARIABLE_ARGUMENT){
            if (atoi(Oper->Value.data()) > ArgumentRegisters.size()){
                // TODO GET FROM STACK
                std::cerr << __LINE__ << " STUB" << std::endl;
            }
            Assembly += "    push " + ArgumentRegisters[std::atoi(Oper->Value.data())] + "\n";
            Stack->Push();
            Types.push(Oper->TypeDef);
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_FUNCTION){
            // Sort out saving current args
            int ParamCount = 0;
            for(auto x : Function->Parameters){
                Assembly += "    push " + ArgumentRegisters[ParamCount] + "\n";
                Stack->Push();
            }

            for(auto p : Oper->Values){
                std::string Parameter = InterpretOperation(File, Function, p, ParentBlock);
                if(Parameter == ""){
                    return "";
                }
                Assembly += Parameter;
            }

            // Save stack for allignment
            //TODO Stack helper tool to do this and allow stack based argumetns
            // Assembly += "    mov q [" + Function->Symbol + "__RSP_8], rsp\n    and q rsp, rsp, -64\n";


            Assembly += "    call " + Oper->Value + "\n";

            // Assembly += "    mov q rsp, [" + Function->Symbol + "__RSP_8]\n";

            ParamCount = 0;
            for(auto x : Function->Parameters){
                Assembly += "    pop " + ArgumentRegisters[ParamCount] + "\n";
                Stack->Pop();
            }

            Assembly += "    push rax\n"; // Load return value
            Stack->Push();

            Types.push(Oper->TypeDef);
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_ADD){
            Assembly += "    pop " + Register2->Get(8) + "\n";
            Stack->Pop();
            Assembly += "    pop " + Register1->Get(8) + "\n";
            Stack->Pop();

            // TODO Non-Standard operators!!!

            std::string Reg1Type = Types.top();
            Types.pop();
            std::string Reg2Type = Types.top();
            Types.pop();

            if(StandardisedTypes[Reg1Type]->DataSize > StandardisedTypes[Reg2Type]->DataSize){
                Assembly += "    add " + Register1->GetBitOp(StandardisedTypes[Reg1Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg1Type);
            }
            else{
                Assembly += "    add " + Register1->GetBitOp(StandardisedTypes[Reg2Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg2Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg2Type);
            }
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_SUB){
            Assembly += "    pop " + Register2->Get(8) + "\n";
            Stack->Pop();
            Assembly += "    pop " + Register1->Get(8) + "\n";
            Stack->Pop();

            // TODO Non-Standard operators!!!

            std::string Reg1Type = Types.top();
            Types.pop();
            std::string Reg2Type = Types.top();
            Types.pop();

            if(StandardisedTypes[Reg1Type]->DataSize > StandardisedTypes[Reg2Type]->DataSize){
                Assembly += "    sub " + Register1->GetBitOp(StandardisedTypes[Reg1Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg1Type);
            }
            else{
                Assembly += "    sub " + Register1->GetBitOp(StandardisedTypes[Reg2Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg2Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg2Type);
            }
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_MUL){
            Assembly += "    pop " + Register2->Get(8) + "\n";
            Stack->Pop();
            Assembly += "    pop " + Register1->Get(8) + "\n";
            Stack->Pop();

            // TODO Non-Standard operators!!!

            std::string Reg1Type = Types.top();
            Types.pop();
            std::string Reg2Type = Types.top();
            Types.pop();

            if(StandardisedTypes[Reg1Type]->DataSize > StandardisedTypes[Reg2Type]->DataSize){
                Assembly += "    mul " + Register1->GetBitOp(StandardisedTypes[Reg1Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Types.push(Reg1Type);
            }
            else{
                Assembly += "    mul " + Register1->GetBitOp(StandardisedTypes[Reg2Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg2Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg2Type);
            }
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_DIV){
            Assembly += "    pop " + Register2->Get(8) + "\n";
            Stack->Pop();
            Assembly += "    pop " + Register1->Get(8) + "\n";
            Stack->Pop();

            // TODO Non-Standard operators!!!

            std::string Reg1Type = Types.top();
            Types.pop();
            std::string Reg2Type = Types.top();
            Types.pop();

            if(StandardisedTypes[Reg1Type]->DataSize > StandardisedTypes[Reg2Type]->DataSize){
                Assembly += "    div " + Register1->GetBitOp(StandardisedTypes[Reg1Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg1Type);
            }
            else{
                Assembly += "    div " + Register1->GetBitOp(StandardisedTypes[Reg2Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg2Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg2Type);
            }
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_LSL){
            Assembly += "    pop " + Register2->Get(8) + "\n";
            Stack->Pop();
            Assembly += "    pop " + Register1->Get(8) + "\n";
            Stack->Pop();

            // TODO Non-Standard operators!!!

            std::string Reg1Type = Types.top();
            Types.pop();
            std::string Reg2Type = Types.top();
            Types.pop();

            if(StandardisedTypes[Reg1Type]->DataSize > StandardisedTypes[Reg2Type]->DataSize){
                Assembly += "    lsl " + Register1->GetBitOp(StandardisedTypes[Reg1Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg1Type);
            }
            else{
                Assembly += "    lsl " + Register1->GetBitOp(StandardisedTypes[Reg2Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg2Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg2Type);
            }
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_LSR){
            Assembly += "    pop " + Register2->Get(8) + "\n";
            Stack->Pop();
            Assembly += "    pop " + Register1->Get(8) + "\n";
            Stack->Pop();

            // TODO Non-Standard operators!!!

            std::string Reg1Type = Types.top();
            Types.pop();
            std::string Reg2Type = Types.top();
            Types.pop();

            if(StandardisedTypes[Reg1Type]->DataSize > StandardisedTypes[Reg2Type]->DataSize){
                Assembly += "    lsr " + Register1->GetBitOp(StandardisedTypes[Reg1Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg1Type);
            }
            else{
                Assembly += "    lsr " + Register1->GetBitOp(StandardisedTypes[Reg2Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg2Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg2Type);
            }
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_AND){
            Assembly += "    pop " + Register2->Get(8) + "\n";
            Stack->Pop();
            Assembly += "    pop " + Register1->Get(8) + "\n";
            Stack->Pop();

            // TODO Non-Standard operators!!!

            std::string Reg1Type = Types.top();
            Types.pop();
            std::string Reg2Type = Types.top();
            Types.pop();

            if(StandardisedTypes[Reg1Type]->DataSize > StandardisedTypes[Reg2Type]->DataSize){
                Assembly += "    and " + Register1->GetBitOp(StandardisedTypes[Reg1Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg1Type);
            }
            else{
                Assembly += "    and " + Register1->GetBitOp(StandardisedTypes[Reg2Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg2Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg2Type);
            }
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_OR){
            Assembly += "    pop " + Register2->Get(8) + "\n";
            Stack->Pop();
            Assembly += "    pop " + Register1->Get(8) + "\n";
            Stack->Pop();

            // TODO Non-Standard operators!!!

            std::string Reg1Type = Types.top();
            Types.pop();
            std::string Reg2Type = Types.top();
            Types.pop();

            if(StandardisedTypes[Reg1Type]->DataSize > StandardisedTypes[Reg2Type]->DataSize){
                Assembly += "    or " + Register1->GetBitOp(StandardisedTypes[Reg1Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg1Type);
            }
            else{
                Assembly += "    or " + Register1->GetBitOp(StandardisedTypes[Reg2Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg2Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg2Type);
            }
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_XOR){
            Assembly += "    pop " + Register2->Get(8) + "\n";
            Stack->Pop();
            Assembly += "    pop " + Register1->Get(8) + "\n";
            Stack->Pop();

            // TODO Non-Standard operators!!!

            std::string Reg1Type = Types.top();
            Types.pop();
            std::string Reg2Type = Types.top();
            Types.pop();

            if(StandardisedTypes[Reg1Type]->DataSize > StandardisedTypes[Reg2Type]->DataSize){
                Assembly += "    xor " + Register1->GetBitOp(StandardisedTypes[Reg1Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg1Type);
            }
            else{
                Assembly += "    xor " + Register1->GetBitOp(StandardisedTypes[Reg2Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg2Type]->DataSize) + ", " + Register2->Get(StandardisedTypes[Reg2Type]->DataSize) + "\n";
                Assembly += "    push " + Register1->Get(8) + "\n";
                Stack->Push();
                Types.push(Reg2Type);
            }
        }
        else if(Oper->Type == SemanticOperation::SemanticOperationValue::SemanticOperationTypes::OPERATION_NOT){
            Assembly += "    pop " + Register2->Get(8) + "\n";
            Stack->Pop();

            // TODO Non-Standard operators!!!

            std::string Reg1Type = Types.top();
            Types.pop();

            Assembly += "    not " + Register1->GetBitOp(StandardisedTypes[Reg1Type]->DataSize) + " " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + ", " + Register1->Get(StandardisedTypes[Reg1Type]->DataSize) + "\n";
            Assembly += "    push " + Register1->Get(8) + "\n";
            Stack->Push();
            Types.push(Reg1Type);
        }
        else{
            std::cerr << __LINE__ << " Operation Not implemented" << std::endl;
            return "";
        }
    }

    // Result store
    if(Operation->ResultStore != ""){
        // There is a place to put it
        if(Operation->ResultStore.find_first_of("__") != std::string::npos){
            // Variable / TempVarStore
            Assembly += "    pop " + Register1->Get(8) + "\n    mov " + Register1->GetBitOp(StandardisedTypes[Operation->EvaluatedTypedef]->DataSize) + " " + Operation->ResultStore + ", " + Register1->Get(8) + "\n";
            Stack->Pop();
        }
        else{
            // Register store
            Assembly += "    pop " + Operation->ResultStore + "\n";
            Stack->Pop();
        }
    }

    // Free all registers used
    this->Stack->ReleaseTempRegister(Temp1Symbol + "__" + OperationID, Assembly);
    this->Stack->ReleaseTempRegister(Temp2Symbol + "__" + OperationID, Assembly);

    return Assembly;
}

std::map<SemanticCondition::ConditionTypes, SemanticCondition::ConditionTypes> ConditionInverter = {
    {SemanticCondition::ConditionTypes::EQU_CONDITION, SemanticCondition::ConditionTypes::NEQ_CONDITION},
    {SemanticCondition::ConditionTypes::NEQ_CONDITION, SemanticCondition::ConditionTypes::EQU_CONDITION},
    {SemanticCondition::ConditionTypes::LEQ_CONDITION, SemanticCondition::ConditionTypes::GTH_CONDITION},
    {SemanticCondition::ConditionTypes::GEQ_CONDITION, SemanticCondition::ConditionTypes::LTH_CONDITION},
    {SemanticCondition::ConditionTypes::LTH_CONDITION, SemanticCondition::ConditionTypes::GEQ_CONDITION},
    {SemanticCondition::ConditionTypes::GTH_CONDITION, SemanticCondition::ConditionTypes::LEQ_CONDITION},
};

std::string AssemblyGenerator::GenerateIfTree(SemanticisedFile* File, SemanticFunctionDeclaration* Function, SemanticInstance* s, SemanticStatment* Statement, SemanticBlock* Block, std::string Exitter, std::string Entry){
    std::string Assembly = "";

    for(int i = 0; i < Statement->ParameterConditions.size();i++){
        if(Statement->ParameterConditions[i]->GetType() == 3){
            SemanticCondition* Con = (SemanticCondition*)(Statement->ParameterConditions[i]);
            // Two Stack points
            std::string Operation1 = InterpretOperation(File, Function, (SemanticOperation*)(Con->Operation1), Block);
            std::string Operation2 = InterpretOperation(File, Function, (SemanticOperation*)(Con->Operation2), Block);

            if(Operation1.size() <= 0){
                std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Statement Opperation 1 failed to generate" << std::endl;
                return "";
            }
            if(Operation2.size() <= 0){
                std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Statement Opperation 2 failed to generate" << std::endl;
                return "";
            }

            Assembly += Operation1 + Operation2;

            std::string StatementID = GenerateID();
            std::string Temp1Symbol = GenerateID();
            std::string Temp2Symbol = GenerateID();
            RegisterDefinition* Register1 = this->Stack->GetTempRegister(Temp1Symbol, Assembly, StatementID);
            RegisterDefinition* Register2 = this->Stack->GetTempRegister(Temp2Symbol, Assembly, StatementID);

            Assembly += "    pop " + Register2->Get(StandardisedTypes[Con->Operation2->EvaluatedTypedef]->DataSize) + "\n";
            Stack->Pop();
            Assembly += "    pop " + Register1->Get(StandardisedTypes[Con->Operation1->EvaluatedTypedef]->DataSize) + "\n";
            Stack->Pop();

            std::string TypeDefUsed = "";
            int DataSize = 0;

            if(StandardisedTypes[Con->Operation1->EvaluatedTypedef]->DataSize > StandardisedTypes[Con->Operation2->EvaluatedTypedef]->DataSize){
                TypeDefUsed = Con->Operation1->EvaluatedTypedef;
                DataSize = StandardisedTypes[TypeDefUsed]->DataSize;
            }
            else{
                TypeDefUsed = Con->Operation2->EvaluatedTypedef;
                DataSize = StandardisedTypes[TypeDefUsed]->DataSize;
            }

            if(i < Statement->ParameterConditions.size() - 1){
                if(Statement->ParameterConditions[i+1]->GetType() != 4){
                    std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Expected Boolean Operator but got type: " << Statement->ParameterConditions[i++]->GetType() << std::endl;
                    return "";
                }

                if(((SemanticBooleanOperator*)(Statement->ParameterConditions[i+1]))->Condition == SemanticBooleanOperator::AND_CONDITION_OPERATION){
                    Assembly += "    cmp " + Register1->GetBitOp(DataSize) + " " + std::to_string(ConditionInverter[Con->Condition]) + ", " + Register1->Get(DataSize) + ", " + Register2->Get(DataSize) + "\n";
                    Assembly += "    jc " + Exitter + "\n";
                }
                else if(((SemanticBooleanOperator*)(Statement->ParameterConditions[i+1]))->Condition == SemanticBooleanOperator::OR_CONDITION_OPERATION){
                    Assembly += "    cmp " + Register1->GetBitOp(DataSize) + " " + std::to_string(Con->Condition) + ", " + Register1->Get(DataSize) + ", " + Register2->Get(DataSize) + "\n";
                    Assembly += "    jc " + Entry + "\n";
                }
            }
            else{
                Assembly += "    cmp " + Register1->GetBitOp(DataSize) + " " + std::to_string(ConditionInverter[Con->Condition]) + ", " + Register1->Get(DataSize) + ", " + Register2->Get(DataSize) + "\n";
                Assembly += "    jc " + Exitter + "\n";
            }
        
            this->Stack->ReleaseTempRegister(Temp1Symbol + "__" + StatementID, Assembly);
            this->Stack->ReleaseTempRegister(Temp2Symbol + "__" + StatementID, Assembly);
        }
        else if(Statement->ParameterConditions[i]->GetType() == 4){
            continue; // Skip this (Its just so it doesnt error)
        }
        else if(Statement->ParameterConditions[i]->GetType() == 0){
            // Process a compacted down new tree
            SemanticStatment* NewStatement = (SemanticStatment*)(Statement->ParameterConditions[i]);
            Assembly += NewStatement->Namespace + ":\n";
            
            std::string Exit = NewStatement->Namespace + "__END__";

            if(((SemanticBooleanOperator*)(Statement->ParameterConditions[i+1]))->Condition == SemanticBooleanOperator::AND_CONDITION_OPERATION){
                Assembly += GenerateIfTree(File, Function, NewStatement, NewStatement, Block, Exitter, Exit);
            }
            else if(((SemanticBooleanOperator*)(Statement->ParameterConditions[i+1]))->Condition == SemanticBooleanOperator::OR_CONDITION_OPERATION){
                Assembly += GenerateIfTree(File, Function, NewStatement, NewStatement, Block, Exit, Entry);
            }

            Assembly += Exit + ":\n";
        }
        else{
            std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Invalid Boolean Expression Type (" << Statement->ParameterConditions[i]->GetType() << ")" << std::endl;
            return "";
        }
    }

    return Assembly;
 }

std::string AssemblyGenerator::GenerateBlock(SemanticisedFile* File, SemanticFunctionDeclaration* Function, SemanticBlock* Block){
    std::string Assembly = "";
    bool HasReturned = false;

    SemanticInstance* PriorValue = nullptr;

    Assembly += Stack->SaveToStack(0, 0);

    for(auto v : Block->Variables){
        if(v->TypeDef == "char" || v->TypeDef == "uchar"){
            Assembly += "    mov b [" + v->Namespace + "__" + v->Identifier + "], " + v->Initialiser + "\n";
        }
        else if(v->TypeDef == "short" || v->TypeDef == "ushort"){
            Assembly += "    mov w [" + v->Namespace + "__" + v->Identifier + "], " + v->Initialiser + "\n";
        }
        else if(v->TypeDef == "int" || v->TypeDef == "uint"){
            Assembly += "    mov d [" + v->Namespace + "__" + v->Identifier + "], " + v->Initialiser + "\n";
        }
        else if(v->TypeDef == "long" || v->TypeDef == "ulong"){
            Assembly += "    mov q [" + v->Namespace + "__" + v->Identifier + "], " + v->Initialiser + "\n";
        }
        else{
            Assembly += "    ; Unsupported Data Type for Initialiser";
        }
    }

    for (auto s : Block->Block){
        if(s->GetType() == 0){ // Statement
            SemanticStatment* Statement = (SemanticStatment*)s;
            if(Statement->StateType == SemanticStatment::StatementType::RETURN_STATEMENT){
                HasReturned = true;
                if(Statement->ParameterOperations.size() > 0){
                    std::string Operation = InterpretOperation(File, Function, (SemanticOperation*)(Statement->ParameterOperations[0]), Block);

                    if(Operation.size() <= 0){
                        std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Statement Opperation failed to generate" << std::endl;
                        return "";
                    }

                    Assembly += Operation;
                }
                
                // Assembly += "    mov q rsp, [" + Function->Symbol + "__RSP]\n    ret\n";
                Assembly += "    ret\n";
            }
            else if(Statement->StateType == SemanticStatment::IF_STATEMENT){
                Assembly += GenerateIfTree(File, Function, s, Statement, Block, Statement->Block->Namespace + "__IF_Block_End__", Statement->Block->Namespace + "__Entry__");

                Assembly += Statement->Block->Namespace + "__Entry__:\n";
                Assembly += GenerateBlock(File, Function, (SemanticBlock*)(Statement->Block));
                Assembly += "    jmp " + Statement->Block->Namespace + "__Selection_End__\n";
                Assembly += Statement->Block->Namespace + "__IF_Block_End__:\n";
                Assembly += Statement->Block->Namespace + "__Selection_End__:\n";
            }
            else if(Statement->StateType == SemanticStatment::ELSE_STATEMENT){
                if(!PriorValue || PriorValue->GetType() != 0 || (((SemanticStatment*)PriorValue)->StateType != SemanticStatment::IF_STATEMENT && ((SemanticStatment*)PriorValue)->StateType != SemanticStatment::ELSE_IF_STATEMENT)){
                    std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Else Statement cannot be self-managed, must have preceeding IF or ELSE IF" << std::endl;
                    return "";
                }

                std::string IF_END_STRING = Assembly.substr((Assembly.substr(0, Assembly.size() - 1)).find_last_of('\n') + 1); // Break off last line
                Assembly = Assembly.substr(0, Assembly.size() - IF_END_STRING.size());

                // Generate Else
                Assembly += Statement->Block->Namespace + "__Entry__:\n";
                Assembly += GenerateBlock(File, Function, (SemanticBlock*)(Statement->Block));
                Assembly += Statement->Block->Namespace + "__End__:\n";
                Assembly += IF_END_STRING;
            }
            else if(Statement->StateType == SemanticStatment::ELSE_IF_STATEMENT){
                std::string IF_END_STRING = Assembly.substr((Assembly.substr(0, Assembly.size() - 1)).find_last_of('\n') + 1); // Break off last line
                Assembly = Assembly.substr(0, Assembly.size() - IF_END_STRING.size());

                Assembly += GenerateIfTree(File, Function, s, Statement, Block, Statement->Block->Namespace + "__IF_Block_End__");
                
                Assembly += Statement->Block->Namespace + "__Entry__:\n";
                Assembly += GenerateBlock(File, Function, (SemanticBlock*)(Statement->Block));
                Assembly += "    jmp " + IF_END_STRING.substr(0, IF_END_STRING.size() - 2) + "\n";
                Assembly += Statement->Block->Namespace + "__ELSE_IF_Block_End__:\n";

                Assembly += IF_END_STRING;
            }
            else if(Statement->StateType == SemanticStatment::WHILE_STATEMENT){
                Assembly += Statement->Block->Namespace + "__CONDITION_START__:\n";
                Assembly += GenerateIfTree(File, Function, s, Statement, Block, Statement->Block->Namespace + "__WHILE_Block_End__", Statement->Block->Namespace + "__Entry__");

                Assembly += Statement->Block->Namespace + "__Entry__:\n";
                Assembly += GenerateBlock(File, Function, (SemanticBlock*)(Statement->Block));
                Assembly += "    jmp " + Statement->Block->Namespace + "__CONDITION_START__\n";
                Assembly += Statement->Block->Namespace + "__WHILE_Block_End__:\n";
                Assembly += Statement->Block->Namespace + "__WHILE_End__:\n";
            }
            else if(Statement->StateType == SemanticStatment::FOR_STATEMENT){
                // Reset Variables
                for(auto v : ((SemanticBlock*)(Statement->Block))->Variables){
                    if(v->TypeDef == "char" || v->TypeDef == "uchar"){
                        Assembly += "    mov b [" + v->Namespace + "__" + v->Identifier + "], " + v->Initialiser + "\n";
                    }
                    else if(v->TypeDef == "short" || v->TypeDef == "ushort"){
                        Assembly += "    mov w [" + v->Namespace + "__" + v->Identifier + "], " + v->Initialiser + "\n";
                    }
                    else if(v->TypeDef == "int" || v->TypeDef == "uint"){
                        Assembly += "    mov d [" + v->Namespace + "__" + v->Identifier + "], " + v->Initialiser + "\n";
                    }
                    else if(v->TypeDef == "long" || v->TypeDef == "ulong"){
                        Assembly += "    mov q [" + v->Namespace + "__" + v->Identifier + "], " + v->Initialiser + "\n";
                    }
                    else{
                        Assembly += "    ; Unsupported Data Type for Initialiser";
                    }
                }

                Assembly += GenerateBlock(File, Function, ((SemanticBlock*)(Statement->Block)));
            }
            else{
                std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Statement not implemented (" << Statement->StateType << ")" << std::endl;
                return "";
            }
        }
        else if(s->GetType() == 1) { // Operation
            // Need to be sure it has a resultstore
            SemanticOperation* Operation = (SemanticOperation*)s;
            if(Operation->ResultStore.size() == 0){
                std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Failed to interpret operation element as there was no result store" << std::endl;
                return "";
            }
            else{
                std::string OperationS = InterpretOperation(File, Function, Operation, Block);

                if(OperationS.size() <= 0){
                    std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Opperation failed to generate" << std::endl;
                    return "";
                }

                Assembly += OperationS;
            }
        }
        else{
            std::cerr << Function->Identifier << ":" << s->TokenIndex << " << ASM::" << __LINE__ << " << " << "Failed to interpret block element " << s->GetType() << std::endl;
            return "";
        }

        PriorValue = s;
    }

    if(!HasReturned && Function->Block == Block /*Make sure we are in main block for function return condition*/){
        std::cerr << Function->Identifier << " << ASM::" << __LINE__ << " << " << "Function did not return" << std::endl;
        return "";
    }

    Assembly += Stack->RestoreStack();

    return Assembly;
}

std::string AssemblyGenerator::Generate(Parsing::SemanticAnalyser* Files){
    std::string File_Header = ";; This was automatically generated using the Uni-C Compiler for Uni-CPU\n";
    std::string File_BSS = ""; // Un-Initialised Data (Zeroed)
    std::string File_DATA = ""; // Initialised Data
    std::string File_Text = ""; // Code and Consts

    //
    // Load global Variables
    //

    //
    // Load Constants (Non-Global)
    //

    { // Load Globals
        
    }

    //
    // Allocate BSS for (Non-Global) Non-Standard Types
    //

    //
    // Load External Functions
    //

    //
    // Load Functions
    //
    std::string LoadedFunctions = "";
    {
        for(auto F : Files->GetFiles()){
            for(auto f : F->FunctionDefs){

                std::string FunctionString = "";
                
                // Load function
                FunctionString = GenerateBlock(F, f, f->Block);

                if(FunctionString == ""){
                    std::cerr << F->AssociatedFile << ":" << f->Identifier << " << ASM::" << __LINE__ << " << " << "Failed to build function" << std::endl;
                    // return ""; // TODO Enable
                }

                // Load header
                FunctionString = "; global " + f->Symbol + "\n" + f->Symbol + ":\n" + FunctionString;
                // FunctionString = "; global " + f->Symbol + "\n" + f->Symbol + ":\n    mov q [" + f->Symbol + "__RSP], rsp\n" + FunctionString;
                // FunctionString = "; global __" + F->ID + "__" + f->Symbol + "\n__" + F->ID + "__" + f->Symbol + ":\n" + FunctionString;
                // File_BSS += f->Symbol + "__RSP:\n    resb 8\n";
                // File_BSS += f->Symbol + "__RSP_8:\n    resb 8\n";

                // Load Defined Variables
                std::stack<SemanticBlock*> blockStack;
                blockStack.push(f->Block);

                while(!blockStack.empty()){
                    SemanticBlock* CurrentBlock = blockStack.top();
                    blockStack.pop();

                    for(auto v : CurrentBlock->Variables){
                        File_DATA += v->Namespace + "__" + v->Identifier + ":\n";
                        if(v->Identifier.size() > 0){
                            if(v->TypeDef == "char" || v->TypeDef == "uchar"){
                                File_DATA += "    db " + v->Initialiser + "\n";
                            }
                            else if(v->TypeDef == "short" || v->TypeDef == "ushort"){
                                File_DATA += "    dw " + v->Initialiser + "\n";
                            }
                            else if(v->TypeDef == "int" || v->TypeDef == "uint"){
                                File_DATA += "    dd " + v->Initialiser + "\n";
                            }
                            else if(v->TypeDef == "long" || v->TypeDef == "ulong"){
                                File_DATA += "    dq " + v->Initialiser + "\n";
                            }
                            else{
                                File_DATA += "    ; Unsupported Data Type for Initialiser";
                            }
                        }
                        else{
                            File_DATA += "    resb " + std::to_string(StandardisedTypes[v->TypeDef]->DataSize);
                        }
                    }

                    for(auto x : CurrentBlock->Block){
                        if(x->GetType() == 0){
                            if((((SemanticStatment*)x)->Block))
                                blockStack.push(((SemanticBlock*)(((SemanticStatment*)x)->Block)));
                        }
                    }
                }

                LoadedFunctions += FunctionString;
            }
        }
    }
    File_Text += LoadedFunctions;

    //
    // Final Assembly stages
    //

    if(File_BSS.size() + File_DATA.size() + File_Text.size() <= 0){
        std::cerr << "ASM::" << __LINE__ << " << " << "No File Contents Generated" << std::endl;
        // return ""; // TODO ENABLE
    }

    //
    // Entry Function
    //

    std::string EntryText = "";
    // Sort out entry to the program
    {
        // Locate entry function
        std::string EntryFunction = "";
        std::string RetVal = "";
        { // Find entry function in files (First come first served) (Match function style)
            for(auto F : Files->GetFiles()){
                for(auto f : F->FunctionDefs){
                    if(f->Identifier == "main" && f->Namespace == ("__" + F->ID + "____GLOB__") && f->StartToken && f->Parameters.size() == 0 /*TODO Arg stuff*/){
                        if(!f->FunctionReturn.WillReturn){
                            RetVal = "\n    mov q rax, 0";
                        }
                        else if(f->FunctionReturn.TypeDef == "int"){
                            // Nothing here just a preventative method
                        }
                        else continue;
                        // Suitable
                        EntryFunction = f->Symbol;
                        break;
                    }
                }

                if(EntryFunction != ""){
                    break;
                }
            }
        }

        // Sort out stack allignment
        // std::string EntryRSPSave = GenerateID() + "__RSP__STORE";
        // File_BSS = "\n" + EntryRSPSave + ":\n    resb 8\n" + File_BSS;


        EntryText = "; global _start\n_start:\n    call " + EntryFunction + RetVal + "\n    halt\n"; // TODO Arguments and environment processing
        // EntryText = "; global _start\n_start:\n    mov q [" + EntryRSPSave + "], rsp\n    and q rsp, rsp, -16\n    call " + EntryFunction + RetVal + "\n    mov q rsp, [" + EntryRSPSave + "]\n    halt\n"; // TODO Arguments and environment processing
    }

    //
    // Data Assembly
    //

    // Optimisers
    // Dissabled due to stack tracking implemented (Fair play past me)
    if(0 == 0){ // Push Pop Optimiser
        // Split File_Text into lines
        std::istringstream fileStream(File_Text);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(fileStream, line)) {
            lines.push_back(line);
        }

        // Iterate through lines and look for push/pop pairs
        std::vector<std::string> modifiedLines = {};
        std::regex pushRegex(R"(\s*push\s+(\S+))");
        std::regex popRegex(R"(\s*pop\s+(\S+))");
        std::smatch match;

        for (size_t i = 0; i < lines.size(); i++) {
            if (std::regex_search(lines[i], match, pushRegex)) {
                // Found push
                std::string PushReg = match[1];

                if(i == lines.size() - 1){
                    modifiedLines.push_back(lines[i]);
                    continue;
                }

                // Is next line a pop
                if(std::regex_search(lines[i+1], match, popRegex)){
                    // Handle it differently
                    std::string PopReg = match[1];

                    if(PopReg != PushReg){
                        // Only need to add content if registers are different
                        modifiedLines.push_back("    mov q " + PopReg + ", " + PushReg);
                    }
                    i++;
                }
                else{
                    modifiedLines.push_back(lines[i]);
                }
            } else {
                // Keep lines that are not part of push/pop or their replacements
                modifiedLines.push_back(lines[i]);
            }
        }

        // Compile the modified lines back into File_Text
        std::ostringstream modifiedFileText;
        for (const std::string& modified_line : modifiedLines) {
            modifiedFileText << modified_line << "\n";
        }
        File_Text = modifiedFileText.str();
    }

    { // Jump Optimiser
        // Split File_Text into lines
        std::istringstream fileStream(File_Text);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(fileStream, line)) {
            lines.push_back(line);
        }

        // Iterate through lines and look for 'jmp x' or 'jc x' followed by 'x:'
        std::vector<std::string> modifiedLines = {};
        std::regex jmpRegex(R"(\s*j(mp|c)\s+(\S+))");
        std::regex labelRegex(R"(\s*(\S+):)");
        std::smatch match;

        for (size_t i = 0; i < lines.size(); i++) {
            if (std::regex_search(lines[i], match, jmpRegex)) {
                // Found jmp or jc
                std::string jumpTarget = match[2];

                if (i < lines.size() - 1 && std::regex_search(lines[i + 1], match, labelRegex)) {
                    // Found label directly following the jmp/jc
                    std::string label = match[1];
                    if (label == jumpTarget) {
                        // Skip the jmp/jc line if it directly jumps to the next label
                        continue;
                    }
                }
            }
            // Keep all other lines
            modifiedLines.push_back(lines[i]);
        }

        // Compile the modified lines back into File_Text
        std::ostringstream modifiedFileText;
        for (const std::string& modified_line : modifiedLines) {
            modifiedFileText << modified_line << "\n";
        }
        File_Text = modifiedFileText.str();
    }

    { // Mov Chaining Optimiser
        // Split File_Text into lines
        std::istringstream fileStream(File_Text);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(fileStream, line)) {
            lines.push_back(line);
        }

        // Iterate through lines and look for 'mov q x, y' followed by 'mov q y, z'
        std::vector<std::string> modifiedLines = {};
        std::regex movRegex(R"(\s*mov\s+(q|d|w|b)\s+(\S+),\s+(\S+))");
        std::smatch match;

        for (size_t i = 0; i < lines.size(); i++) {
            if (std::regex_search(lines[i], match, movRegex)) {
                // Found first mov
                std::string firstSize = match[1];
                std::string firstDest = match[2];
                std::string firstSrc = match[3];

                if (i < lines.size() - 1 && std::regex_search(lines[i + 1], match, movRegex)) {
                    // Found second mov
                    std::string secondSize = match[1];
                    std::string secondDest = match[2];
                    std::string secondSrc = match[3];

                    if (firstDest == secondSrc) {
                        // Combine the two mov instructions
                        modifiedLines.push_back("    mov " + secondSize + " " + secondDest + ", " + firstSrc);
                        i++; // Skip the second mov line
                        continue;
                    }
                }
            }
            // Keep all other lines
            modifiedLines.push_back(lines[i]);
        }

        // Compile the modified lines back into File_Text
        std::ostringstream modifiedFileText;
        for (const std::string& modified_line : modifiedLines) {
            modifiedFileText << modified_line << "\n";
        }
        File_Text = modifiedFileText.str();
    }

    { // Incr Optimisation (+1)
        // Split File_Text into lines
        std::istringstream fileStream(File_Text);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(fileStream, line)) {
            lines.push_back(line);
        }

        std::vector<std::string> modifiedLines = {};
        std::regex matchRegex(R"(/\s*add\sq\s(r.), (r.), 1/gm)"); // TODO Think about this optimisation in terms of bitsizes?
        std::smatch match;

        for(auto l : lines){
            if(std::regex_search(l, match, matchRegex)){
                std::string Arg1 = match[1];
                std::string Arg2 = match[2];
                std::string Arg3 = match[3];

                if(Arg1 == Arg2){
                    modifiedLines.push_back(std::string("    incr ") + Arg1);
                }
                else{
                    modifiedLines.push_back(l);
                }

            }
            else{
                modifiedLines.push_back(l);
            }
        }
    }

    { // Decr Optimisation (-1)
        // Split File_Text into lines
        std::istringstream fileStream(File_Text);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(fileStream, line)) {
            lines.push_back(line);
        }

        std::vector<std::string> modifiedLines = {};
        std::regex matchRegex(R"(/\s*add\sq\s(r.), (r.), 1/gm)"); // TODO Think about this optimisation in terms of bitsizes?
        std::smatch match;

        for(auto l : lines){
            if(std::regex_search(l, match, matchRegex)){
                std::string Arg1 = match[1];
                std::string Arg2 = match[2];
                std::string Arg3 = match[3];

                if(Arg1 == Arg2){
                    modifiedLines.push_back(std::string("    decr ") + Arg1);
                }
                else{
                    modifiedLines.push_back(l);
                }
            }
            else{
                modifiedLines.push_back(l);
            }
        }
    }

    { // Inc Optimisation (+x)
        // Split File_Text into lines
        std::istringstream fileStream(File_Text);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(fileStream, line)) {
            lines.push_back(line);
        }

        std::vector<std::string> modifiedLines = {};
        std::regex matchRegex(R"(/\s*add\sq\s(r.), (r.), (\S+)/gm)"); // TODO Think about this optimisation in terms of bitsizes?
        std::smatch match;

        for(auto l : lines){
            if(std::regex_search(l, match, matchRegex)){
                std::string Arg1 = match[1];
                std::string Arg2 = match[2];
                std::string Arg3 = match[3];

                if(Arg1 == Arg2){
                    modifiedLines.push_back(std::string("    inc ") + Arg1 + ", " + Arg3);
                }
                else{
                    modifiedLines.push_back(l);
                }

            }
            else{
                modifiedLines.push_back(l);
            }
        }
    }

    { // Dec Optimisation (-x)
        // Split File_Text into lines
        std::istringstream fileStream(File_Text);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(fileStream, line)) {
            lines.push_back(line);
        }

        std::vector<std::string> modifiedLines = {};
        std::regex matchRegex(R"(/\s*sub\sq\s(r.), (r.), (\S+)/gm)"); // TODO Think about this optimisation in terms of bitsizes?
        std::smatch match;

        for(auto l : lines){
            if(std::regex_search(l, match, matchRegex)){
                std::string Arg1 = match[1];
                std::string Arg2 = match[2];
                std::string Arg3 = match[3];

                if(Arg1 == Arg2){
                    modifiedLines.push_back(std::string("    dec ") + Arg1 + ", " + Arg3);
                }
                else{
                    modifiedLines.push_back(l);
                }

            }
            else{
                modifiedLines.push_back(l);
            }
        }
    }

    return File_Header + "; TODO REMOVE AS EXECUTABLE SHOULD PRELOAD RSP\n    mov q rsp, 50000000\n    jmp _start\n; section .bss\n" + File_BSS + "\n; section .data\n" + File_DATA + "\n; section .text\n" + EntryText + File_Text;
}

AssemblyGenerator::AssemblyGenerator(){
    // Setup ; global stack
    this->Stack = new GlobalStack{};

    // Registers

    this->Stack->SymbolMap = {
        {new RegisterDefinition{{{64, "r0"}, {32, "r0"}, {16, "r0"}, {8, "r0"}}}, ""},
        {new RegisterDefinition{{{64, "r1"}, {32, "r1"}, {16, "r1"}, {8, "r1"}}}, ""},
        {new RegisterDefinition{{{64, "r2"}, {32, "r2"}, {16, "r2"}, {8, "r2"}}}, ""},
        {new RegisterDefinition{{{64, "r3"}, {32, "r3"}, {16, "r3"}, {8, "r3"}}}, ""},
        {new RegisterDefinition{{{64, "r4"}, {32, "r4"}, {16, "r4"}, {8, "r4"}}}, ""},
        {new RegisterDefinition{{{64, "r5"}, {32, "r5"}, {16, "r5"}, {8, "r5"}}}, ""},
        {new RegisterDefinition{{{64, "r6"}, {32, "r6"}, {16, "r6"}, {8, "r6"}}}, ""},
        {new RegisterDefinition{{{64, "rax"}, {32, "rax"}, {16, "rax"}, {8, "rax"}}}, ""},
        {new RegisterDefinition{{{64, "rbx"}, {32, "rbx"}, {16, "rbx"}, {8, "rbx"}}}, ""},
        {new RegisterDefinition{{{64, "rcx"}, {32, "rcx"}, {16, "rcx"}, {8, "rcx"}}}, ""},
        {new RegisterDefinition{{{64, "rdx"}, {32, "rdx"}, {16, "rdx"}, {8, "rdx"}}}, ""},
        {new RegisterDefinition{{{64, "rsi"}, {32, "rsi"}, {16, "rsi"}, {8, "rsi"}}}, ""},
        {new RegisterDefinition{{{64, "rdi"}, {32, "rdi"}, {16, "rdi"}, {8, "rdi"}}}, ""},
        {new RegisterDefinition{{{64, "rsp"}, {32, "rsp"}, {16, "rsp"}, {8, "rsp"}}}, ""},
    };
}