

#ifndef STATE_H
#define STATE_H

#include "uthash.h"

typedef enum {
	ET_STRING,
	ET_INTEGER,
	ET_DECIMAL,
	ET_VECTOR
} ElementType;

typedef enum {
	TT_OP,
	TT_INT,
	TT_FLOAT,
	TT_STRING,
	
	TT_DEFINE
} TokenType;

/*
typedef struct Operator Operator;
struct Operator {
	char *s;
	// TODO: Add pointer to defenition
	
	UT_hash_handle hh;
};*/


typedef struct MemoryPool MemoryPool;
struct MemoryPool {
	void *members;
	unsigned int nextSlot;
	unsigned int capacity;
	unsigned int freeCount;
	
	MemoryPool *next;
	MemoryPool *prev;
};



typedef struct Token Token;
struct Token {
	char *s;
	Token *next;
	TokenType type;
};

typedef struct TokenDefine TokenDefine;
struct TokenDefine {
	char *s;
	unsigned int mid;
};

typedef struct Method Method;
struct Method{
	Token *start;
	unsigned int mid;
	int inUse;
};
typedef struct MethodList MethodList;
struct MethodList {
	char *name;
	unsigned int mid;
	UT_hash_handle hh;
};

typedef struct Element Element;
struct Element{
	ElementType type;
	void *data;
	double dval;
	long int ival;
	MethodList *methods;
	MemoryPool *owner;
	
	//Operator *operators;
};

typedef struct Vector Vector;
struct Vector {
	unsigned int len;
	unsigned int alloc;
	Element **data;
};

typedef struct Stack Stack;
struct Stack {
	Element *elm;
	Stack *next;
	unsigned int bRef;
};


typedef struct SymbolTable SymbolTable;
struct SymbolTable {
	char *name;
	Element *elm;
	
	UT_hash_handle hh;
};

typedef struct State State;
struct State {
	int invalid;
	char *errStr;
	MethodList *globalMethods;
	Stack *stack;
	
	Stack *vstack;
	
	SymbolTable *symbols;
	
	// Token list
	// Stack
	// variable symbol table
	// global defined operator list
};


#endif 

