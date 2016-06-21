#include "methods.h"

void freeElement(Element *elm){
	if(elm == NULL)
		return;
	
	if(elm->data != NULL){
		if(elm->type == ET_VECTOR){
			freeVector((Vector*)elm->data);
		}else{
			free(elm->data);
		}
	}
	
	freeMethodList(&elm->methods);
	free(elm);
}

Element *newElement(ElementType type, void *data){
	Element *elm = malloc(sizeof(Element));
	elm->type = type;
	elm->data = data;
	elm->methods = newMethodList();
	return elm;
}

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

Element *dupElement(Element *elm){
	Element *ret = newElement(elm->type, NULL);
	if(ret->type == ET_STRING){
		ret->data = (void*)dup((char*)elm->data);
	}else if(ret->type == ET_INTEGER){
		ret->data = malloc(sizeof(long int));
		*((long int*)ret->data) = *((long int*)elm->data);
	}else if(ret->type == ET_DECIMAL){
		ret->data = malloc(sizeof(double));
		*((double*)ret->data) = *((double*)elm->data);
	}else if(ret->type == ET_VECTOR){
		Vector *v = (Vector*)elm->data;
		Vector *vnew = newVector();
		for(int i=0; i<v->len; ++i){
			vectorPushBack(vnew, dupElement(v->data[i]));
		}
		ret->data = (void*)vnew;
	}
	
	cloneOps(ret, elm);
	return ret;
}

Token *mqlProc_Elm(State *s, Token *tk){
	Element *elm = NULL;
	
	if(tk->type == TT_INT){
		long int *x = malloc(sizeof(long int));
		*x = strtol(tk->s, NULL, 0);
		elm = newElement(ET_INTEGER, (void*)x);
	}else if(tk->type == TT_FLOAT){
		double *x = malloc(sizeof(double));
		*x = strtod(tk->s, NULL);
		elm = newElement(ET_DECIMAL, (void*)x);
	}else if(tk->type == TT_STRING){
		elm = newElement(ET_STRING, (void*)dup(tk->s));
	}else{
		s->invalid = 1;
		s->errStr = dup("Invalid token reached in mqlProc_Elm");
	}
	s->stack = stackPush(s->stack, elm);
	return tk->next;
}

void printElement(Element *elm){
	if(elm == NULL){
		printf("( NULL )\n");
	}else if(elm->type == ET_INTEGER){
		printf("%ld \n", *(long int*)elm->data);
	}else if(elm->type == ET_DECIMAL){
		printf("%f \n", *(double*)elm->data);
	}else if(elm->type == ET_STRING){
		printf("'%s' \n", (char*)elm->data);
	}else if(elm->type == ET_VECTOR){
		Vector *v = (Vector*)elm->data;
		printf("<  \n");
		for(int i=0; i<v->len; ++i){
			printf("   ");
			printElement(v->data[i]);
		}
		printf(">\n");
	}else{
		printf(" [ ??? ] \n");
	}
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

Token* opPrint(State* s, Token* tk){
	Element *top = stackPoll(s->stack);
	printElement(top);
	return tk;
}

void registerElementOps(){
	registerGloablOp("[", &opVecOpen);
	registerGloablOp("]", &opVecClose);
	registerGloablOp(",", &opVecPack);
	registerGloablOp("@", &opPrint);
}


