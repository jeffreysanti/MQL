#include "methods.h"

#define POOLSIZE_ELM 1024

MemoryPool *pool_elm;
unsigned int pool_elm_cnt;

void startNewElementPool(){
	pool_elm_cnt ++;
	MemoryPool *pool = malloc(sizeof(MemoryPool));
	pool->members = malloc(POOLSIZE_ELM * sizeof(Element));
	pool->capacity = POOLSIZE_ELM;
	pool->freeCount = 0;
	pool->nextSlot = 0;
	pool->prev = pool_elm;
	if(pool_elm != NULL){
		pool_elm->next = pool;
	}
	pool_elm = pool;
}

void freeElement(Element *elm){
	if(elm == NULL){
		return;
	}
	
	MemoryPool *owner = elm->owner;
	++ owner->freeCount;
	if(owner->freeCount == owner->capacity){
		pool_elm_cnt --;
		for(unsigned int i=0; i<owner->capacity; ++i){
			Element elm = ((Element*)owner->members)[i];
			if(elm.data != NULL){
				if(elm.type == ET_VECTOR){
					freeVector((Vector*)elm.data);
				}else{
					free(elm.data);
				}
			}
			
			freeMethodList(&elm.methods);
		}
		
		free(owner->members);
		if(owner == pool_elm){
			startNewElementPool();
			owner->next = pool_elm;
			
		}
		if(owner->prev != NULL){
			owner->prev->next = owner->next;
		}
		if(owner->next != NULL){
			owner->next->prev = owner->prev;
		}
		free(owner);
	}
}

Element *newElement(ElementType type, void *data){
	if(pool_elm == NULL || pool_elm->nextSlot == pool_elm->capacity){
		startNewElementPool();
	}
	
	Element *elm = &((Element*)pool_elm->members)[pool_elm->nextSlot];
	++ pool_elm->nextSlot;
	elm->type = type;
	elm->data = data;
	elm->dval = 0;
	elm->ival = 0;
	elm->methods = newMethodList();
	elm->owner = pool_elm;
	return elm;
}

void freeElementPool(){
	
}


Buffer *newBuffer(){
	Buffer *b = malloc(sizeof(Buffer));
	b->type = 0;
	b->extra = NULL;
	b->eob = 0;
	b->next = NULL;
	b->free = NULL;
}

void freeBuffer(Buffer *b){
	if(b == NULL)
		return;
	
	if(b->free != NULL){
		b->free(b);
	}
	free(b);
}



Element *dupElement(Element *elm){
	Element *ret = newElement(elm->type, NULL);
	if(ret->type == ET_STRING){
		ret->data = (void*)dup((char*)elm->data);
	}else if(ret->type == ET_INTEGER || ret->type == ET_DECIMAL){
		ret->dval = elm->dval;
		ret->ival = elm->ival;
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
		long int x = strtol(tk->s, NULL, 0);
		elm = newElement(ET_INTEGER, NULL);
		elm->ival = x;
	}else if(tk->type == TT_FLOAT){
		double x = strtod(tk->s, NULL);
		elm = newElement(ET_DECIMAL, NULL);
		elm->dval = x;
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
		printf("%ld \n", elm->ival);
	}else if(elm->type == ET_DECIMAL){
		printf("%f \n", elm->dval);
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


Token* opPrint(State* s, Token* tk){
	Element *top = stackPoll(s->stack);
	printElement(top);
	return tk;
}

void registerElementOps(){
	registerGloablOp("@", &opPrint);
}


