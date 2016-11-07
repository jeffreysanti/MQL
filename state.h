

#ifndef STATE_H
#define STATE_H

#include "uthash.h"

typedef enum {
	ET_STRING,
	ET_NUMBER,
	ET_VECTOR,
	
	ET_BUFFER
} ElementType;

typedef enum {
	TT_OP,
	TT_NUMBER,
	TT_STRING,
	TT_CODEBLOCK,
	
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
	unsigned int cbid;
};

typedef struct CodeBlock CodeBlock;
struct CodeBlock{
	Token *start;
	unsigned int cbid;
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

typedef struct Buffer Buffer;
struct Buffer {
	int type;
	void *extra;
	char eob;
	Buffer *sourceBuffer1; // where this buffer pulls data from
	Buffer *sourceBuffer2; // where this buffer pulls data from
	Buffer *firstBuffer; // original source of data
	
	Element *lastData;
	unsigned int syncCounter;
	
	void (*next)(Buffer*);
	void (*free)(Buffer*);
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

