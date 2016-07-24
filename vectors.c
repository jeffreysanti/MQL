#include "methods.h"


Vector *newVector(){
	Vector *v = malloc(sizeof(Vector));
	v->len = 0;
	v->alloc = 0;
	v->data = NULL;
	return v;
}

void freeVector(Vector *v){
	if(v == NULL)
		return;
	
	if(v->data != NULL){
		for(int i=0; i<v->len; ++i){
			freeElement(v->data[i]);
		}
		free(v->data);
	}
	free(v);
}

void vectorPushBack(Vector *v, Element *e){
	if(e == NULL){
		return;
	}
	
	v->len = v->len + 1;
	if(v->alloc == 0){
		v->data = malloc(sizeof(Element*));
		v->alloc = 1;
	}
	
	if(v->len > v->alloc){ // need more space
		v->alloc = v->alloc * 2;
		v->data = realloc(v->data, v->alloc * sizeof(Element*));
	}
	v->data[v->len-1] = e;
}



// if a vector element is on the stack does nothing
// otherwise autopack all elements on stack into
// a vector on the stack
void autoPackStack(State* s){
	Element *top = stackPoll(s->stack);
	if(top == NULL){
		top = newElement(ET_VECTOR, (void*)newVector());
		s->stack = stackPush(s->stack, top);
		return;
	}
	if(top->type == ET_VECTOR){
		return; // we're all good
	}
	
	int elmCnt = stackSize(s->stack);
	Vector *v = newVector();
	v->alloc = elmCnt;
	v->len = elmCnt;
	v->data = malloc(sizeof(Element*) * elmCnt);
	Element *ret = newElement(ET_VECTOR, (void*)v);
	
	for(int i=elmCnt-1; i>=0; --i){
		Element *e = stackPoll(s->stack);
		v->data[i] = e;
		s->stack = stackPopNoFree(s->stack);
	}
}

Token* opVecOpen(State* s, Token* tk){
	Element *velm = newElement(ET_VECTOR, (void*)newVector());
	s->vstack = stackPush(s->vstack, velm);
	
	if(tk != NULL && tk->type == TT_OP && !strcmp(tk->s, "]")){ // empty vector
		s->vstack = stackPopNoFree(s->vstack);
		s->stack = stackPush(s->stack, velm);
		return tk->next;
	}
	
	return tk;
}

Token* opVecPack(State* s, Token* tk){
	Element *velm = stackPoll(s->vstack);
	if(velm == NULL){
		s->invalid = 1;
		s->errStr = dup("Pack stack without openning");
		return tk;
	}
	Vector *v = (Vector*)velm->data;
	
	Element *op = stackPoll(s->stack);
	if(op == NULL){ // push an empty vector
		op = newElement(ET_VECTOR, (void*)newVector());
	}else{
		s->stack = stackPopNoFree(s->stack);
	}
	
	vectorPushBack(v, op);
	return tk;
}

Token* opVecClose(State* s, Token* tk){
	Element *velm = stackPoll(s->vstack);
	if(velm == NULL){
		s->invalid = 1;
		s->errStr = dup("Closed stack without openning");
		return tk;
	}
	if(stackPoll(s->stack) != NULL){
		tk = opVecPack(s, tk); // pack last element onto vector
	}
	s->vstack = stackPopNoFree(s->vstack);
	s->stack = stackPush(s->stack, velm);
	return tk;
}

Token* opRange(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op2 == NULL){
		if(op2 != NULL) freeElement(op2);
		return tk;
	}
	
	if(op2->type != ET_INTEGER || op1->type != ET_INTEGER){
		s->invalid = 1;
		s->errStr = dup("Range must take 2 ints");
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	Vector *vec = newVector();
	Element *push = newElement(ET_VECTOR, (void*)vec);
	if(op1->ival < op2->ival){
		for(int i=op1->ival; i<=op2->ival; ++i){
			Element *e = newElement(ET_INTEGER, NULL);
			e->ival = i;
			vectorPushBack(vec, e);
		}
	}else{
		for(int i=op1->ival; i>=op2->ival; --i){
			Element *e = newElement(ET_INTEGER, NULL);
			e->ival = i;
			vectorPushBack(vec, e);
		}
	}
	
	// push result
	s->stack = stackPush(s->stack, push);
	
	freeElement(op1);
	freeElement(op2);
	return tk;
}

Token* opGet(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = stackPoll(s->stack);
	if(op2 == NULL){
		return tk;
	}
	
	if(op1 == NULL || op2->type != ET_INTEGER || op1->type != ET_VECTOR){
		s->invalid = 1;
		s->errStr = dup("Get must take form <vec> <index:int> get");
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	Vector *v = (Vector*)op1->data;
	int index = op2->ival;
	if(index > v->len || index <= 0){
		s->invalid = 1;
		s->errStr = dup("Get out of range");
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	// push result
	Element *push = dupElement(v->data[index - 1]);
	s->stack = stackPush(s->stack, push);
	
	freeElement(op2);
	return tk;
}

Token* opCount(State* s, Token* tk){
	Element *op1 = stackPoll(s->stack);
	if(op1 == NULL){
		return tk;
	}
	
	if(op1 == NULL || op1->type != ET_VECTOR){
		s->invalid = 1;
		s->errStr = dup("Count must take vector");
		freeElement(op1);
		return tk;
	}
	
	Vector *v = (Vector*)op1->data;
	Element *push = newElement(ET_INTEGER, NULL);
	push->ival = v->len;
	s->stack = stackPush(s->stack, push);
	
	return tk;
}

Token* opSet(State* s, Token* tk){
	Element *op3 = popStackOrErr(s);
	Element *op2 = popStackOrErr(s);
	Element *op1 = stackPoll(s->stack);
	if(op2 == NULL || op3 == NULL){
		return tk;
	}
	
	if(op1 == NULL || op3->type != ET_INTEGER || op1->type != ET_VECTOR){
		s->invalid = 1;
		s->errStr = dup("Get must take form <vec> <val> <index:int> set");
		freeElement(op1);
		freeElement(op2);
		freeElement(op3);
		return tk;
	}
	
	Vector *v = (Vector*)op1->data;
	int index = op3->ival;
	if(index > v->len || index <= 0){
		s->invalid = 1;
		s->errStr = dup("Set out of range");
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	// set action
	Element *old = v->data[index - 1];
	freeElement(old);
	v->data[index - 1] = op2;
	
	freeElement(op3);
	return tk;
}

Token* opVPush(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = stackPoll(s->stack);
	if(op2 == NULL){
		return tk;
	}
	
	if(op1 == NULL || op1->type != ET_VECTOR){
		s->invalid = 1;
		s->errStr = dup("VPUSH must take form <vec> <val> vpush");
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	Vector *v = (Vector*)op1->data;
	vectorPushBack(v, op2);
	
	return tk;
}

void registerVectorOps(){
	registerGloablOp("[", &opVecOpen);
	registerGloablOp("]", &opVecClose);
	registerGloablOp(",", &opVecPack);
	registerGloablOp("RANGE", &opRange);
	registerGloablOp("range", &opRange);
	registerGloablOp("GET", &opGet);
	registerGloablOp("get", &opGet);
	registerGloablOp("COUNT", &opCount);
	registerGloablOp("count", &opCount);
	registerGloablOp("SET", &opSet);
	registerGloablOp("set", &opSet);
	registerGloablOp("VPUSH", &opVPush);
	registerGloablOp("vpush", &opVPush);
}

