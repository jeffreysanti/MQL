#include "methods.h"

Stack *newStack(){
	return NULL;
}

Stack *stackPush(Stack *s, Element *elm){
	Stack *s_new = malloc(sizeof(Stack));
	s_new->elm = elm;
	s_new->next = s;
	s_new->bRef = 0;
	return s_new;
}

Element *stackPoll(Stack *s){
	if(s == NULL)
		return NULL;
	return s->elm;
}

Stack *stackPop(Stack *s){
	if(s == NULL)
		return NULL;
	Stack *ret = s->next;
	s->next = NULL;
	freeElement(s->elm);
	free(s);
	return ret;
}

Stack *stackPopNoFree(Stack *s){
	if(s == NULL)
		return NULL;
	Stack *ret = s->next;
	s->next = NULL;
	free(s);
	return ret;
}

Element *popStackOrErr(State *state){
	if(state->stack == NULL){
		state->invalid = 1;
		state->errStr = dup("Stack is empty");
		return NULL;
	}
	Element *ret = state->stack->elm;
	state->stack = stackPopNoFree(state->stack);
	return ret;
}

Stack *stackPopAll(Stack *s){
	while(s != NULL){
		s = stackPop(s);
	}
	return s;
}

unsigned int stackSize(Stack *s){
	unsigned int ss = 0;
	while(s != NULL){
		s = s->next;
		++ ss;
	}
	return ss;
}

void stackReferenced(Stack *s){
	s->bRef = 1;
}

Element *stackMutated(Stack *s){
	if(s->bRef){
		s->elm = dupElement(s->elm);
		s->bRef = 0;
	}
	return s->elm;
}



Token* opDot(State* s, Token* tk){
	s->stack = stackPop(s->stack);
	return tk;
}

Token* opCLR(State* s, Token* tk){
	s->stack = stackPopAll(s->stack);
	return tk;
}

Token* opDUP(State* s, Token* tk){
	Element *op1 = stackPoll(s->stack);
	if(op1 == NULL){
		s->invalid = 1;
		s->errStr = dup("Cannot dup empty stack");
		return tk;
	}
	
	Element *push = dupElement(op1);
	s->stack = stackPush(s->stack, push);
	return tk;
}

Token* opSWAP(State* s, Token* tk){
	Element *op1 = popStackOrErr(s);
	Element *op2 = popStackOrErr(s);
	Element *push = NULL;
	if(op1 == NULL || op2 == NULL){
		if(op1 != NULL) freeElement(op1);
		return tk;
	}
	
	s->stack = stackPush(s->stack, op1);
	s->stack = stackPush(s->stack, op2);
	return tk;
}

void registerStackOps(){
	registerGloablOp(".", &opDot);
	registerGloablOp("clr", &opCLR);
	registerGloablOp("CLR", &opCLR);
	registerGloablOp("dup", &opDUP);
	registerGloablOp("DUP", &opDUP);
	registerGloablOp("swap", &opSWAP);
	registerGloablOp("SWAP", &opSWAP);
}
