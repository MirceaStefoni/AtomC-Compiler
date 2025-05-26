#pragma once

// stack based virtual machine

typedef enum{
	OP_HALT,
	OP_PUSH_I,
	OP_CALL,
	OP_CALL_EXT,
	OP_ENTER,
	OP_RET,
	OP_RET_VOID,
	OP_CONV_I_F,
	OP_JMP,
	OP_JF,
	OP_JT,
	OP_FPLOAD,
	OP_FPSTORE,
	OP_ADD_I,
	OP_LESS_I,
	OP_PUSH_F,
	OP_CONV_F_I,
	OP_LOAD_I,
	OP_LOAD_F,
	OP_STORE_I,
	OP_STORE_F,
	OP_ADDR,
	OP_FPADDR_I,
	OP_FPADDR_F,
	OP_ADD_F,
	OP_SUB_I,
	OP_SUB_F,
	OP_MUL_I,
	OP_MUL_F,
	OP_DIV_I,
	OP_DIV_F,
	OP_LESS_F,
	OP_DROP,
	OP_NOP,
	OP_LESSEQ_I,     // compares 2 int values from stack (<=) and puts result on stack as int
	OP_LESSEQ_F,     // compares 2 double values from stack (<=) and puts result on stack as int
	OP_GREATER_I,    // compares 2 int values from stack (>) and puts result on stack as int
	OP_GREATER_F,    // compares 2 double values from stack (>) and puts result on stack as int
	OP_GREATEREQ_I,  // compares 2 int values from stack (>=) and puts result on stack as int
	OP_GREATEREQ_F   // compares 2 double values from stack (>=) and puts result on stack as int
	}Opcode;

typedef struct Instr Instr;

typedef union{
	int i;
	double f;
	void *p;
	void(*extFnPtr)();
	Instr *instr;
	}Val;

struct Instr{
	Opcode op;
	Val arg;
	Instr *next;
	};

Instr *addInstr(Instr **list,Opcode op);
Instr *insertInstr(Instr *before,int op);
void delInstrAfter(Instr *instr);
Instr *lastInstr(Instr *list);
Instr *addInstrWithInt(Instr **list,Opcode op,int argVal);
Instr *addInstrWithDouble(Instr **list,Opcode op,double argVal);
void vmInit();
void run(Instr *IP);
Instr *genTestProgram();