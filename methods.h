



#ifndef METHODS_H
#define METHODS_H

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <math.h>
#include "utils.h"
#include "state.h"

// Main
State *mql_s(State *s, const char *str);
State *mql(State *s, Token *tk);
Token *mqlCodeBlock(State *s, Token *tk);
State *newState();
void freeState(State *s);
Token *mql_op(State *s, Token *tk);


// LEXER
Token *tokenize(const char *s);
int isInputComplete(Token *tk);
void freeToken(Token *tk);
Token *duplicateToken(Token *tk);

// METHODS
MethodList *newMethodList();
void freeMethodList(MethodList **ml);
void registerMethodOps();
void initMethodSpace();
void freeMethodSpace();
Token *mqlProc_Def(State *s, Token *tk);
Token* preprocessMethods(State* s, Token* tk);
void addMethod(MethodList **ml, char *s, unsigned int mid);
void removeMethod(MethodList **ml, char *s);
Token *execGloablOp(State *s, Token *tk);
Token *execAssociatedOp(State *s, Token *tk);
Token *findMethod(MethodList *ml, char *s);
SymbolTable *newSymbolTable();
void freeSymbolTable(SymbolTable **st);
void addSymbol(SymbolTable **st, char *s, Element *elm);
Token *codeBlockExecToken(int cbid);


// STACK
Stack *newStack();
Stack *stackPush(Stack *s, Element *elm);
Element *stackPoll(Stack *s);
Stack *stackPop(Stack *s);
Stack *stackPopAll(Stack *s);
Element *popStackOrErr(State *state);
Stack *stackPopNoFree(Stack *s);
void registerStackOps();
unsigned int stackSize(Stack *s);
void stackReferenced(Stack *s);
Element *stackMutated(Stack *s);

// ELEMENT
void freeElement(Element *elm);
void freeElementPool();
Token *mqlProc_Elm(State *s, Token *tk);
Element *newElement(ElementType type, void *data);
void printElement(Element *elm);
Element *dupElement(Element *elm);
void registerElementOps();
void cloneOps(Element *dest, Element *src);
Buffer *newBufferOriginal();
Buffer *newBufferWithSource(Buffer *src1, Buffer *src2);
void freeBuffer(Buffer *b);

// VECTORS
Vector *newVector();
void freeVector(Vector *v);
void vectorPushBack(Vector *v, Element *e);
void autoPackStack(State* s);
void registerVectorOps();
Element *mergeNoDuplicateVectors(Element *e1, Element *e2);

// Buffers
Vector *vectorFromBuffer(State *s, Buffer *buf);
Buffer *bufferFromVector(Vector *vec);
Element *advanceBuffer(State *s, Buffer *buf);
Element *getBufferData(State *s, Buffer *buf);
Buffer *dupBuffer(Buffer *buf);
void registerBufferOps();
int commonBufferLineage(Buffer *b1, Buffer *b2);

// OPS
void registerGloablOp(char *s, Token* (*func)(State*, Token*));
Token *execSuperGloablOp(State *s, Token *tk);
void freeGlobalOps();
void printGlobalOps();

// ARITH
void registerArithmeticOps();

// CONTROL
int isTrue(Element *elm);
void registerControlOps();


// DRIVERS
void driver_sqlite();
void driverfree_sqlite();

#endif

