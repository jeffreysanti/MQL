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
	pool_elm = pool;
}

void freeElement(Element *elm){
	if(elm == NULL){
		return;
	}

	if(elm->data != NULL){
		if(elm->type == ET_VECTOR){
			freeVector((Vector*)elm->data);
		}else if(elm->type == ET_BUFFER){
			freeBuffer((Buffer*)elm->data);
		}else{
			free(elm->data);
		}
	}
	freeMethodList(&elm->methods);
	
	MemoryPool *owner = elm->owner;
	++ owner->freeCount;
	if(owner->freeCount == owner->capacity){
		pool_elm_cnt --;
		free(owner->members);

		if(owner == pool_elm){
			// nothing after it, so go back
			pool_elm = owner->prev;
		}else{
			// need to rebuild the list
			MemoryPool *prev = pool_elm;
			while(prev != NULL){
				if(prev->prev == owner){
					prev->prev = owner->prev;
				}
				prev = prev->prev;
			}
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
	elm->methods = newMethodList();
	elm->owner = pool_elm;
	return elm;
}

void freeElementPool(){
	
}


Buffer *newBufferOriginal(){
	Buffer *b = malloc(sizeof(Buffer));
	b->type = 0;
	b->extra = NULL;
	b->eob = 0;
	b->next = NULL;
	b->free = NULL;
	b->sourceBuffer1 = NULL;
	b->sourceBuffer2 = NULL;
	b->firstBuffer = NULL;
	b->syncCounter = 0;
	b->lastData = NULL;
	b->refCounter = 1;
	return b;
}

Buffer *newBufferWithSource(Buffer *src1, Buffer *src2){
	Buffer *b = malloc(sizeof(Buffer));
	b->type = 0;
	b->extra = NULL;
	b->eob = 0;
	b->next = NULL;
	b->free = NULL;
	b->sourceBuffer1 = src1;
	b->sourceBuffer1->refCounter ++;
	if(src2 != NULL){
		b->sourceBuffer2 = src2;
		b->sourceBuffer2->refCounter ++;
	}else{
		b->sourceBuffer2 = NULL;
	}
	b->refCounter = 1;
	if(src1->firstBuffer == NULL){
		b->firstBuffer = src1;
	}else{
		b->firstBuffer = src1->firstBuffer;
	}
	b->syncCounter = 0;
	b->lastData = NULL;
	return b;
}

void freeBuffer(Buffer *b){
	b->refCounter --;
	if(b->refCounter > 0){
		return;
	}
	if(b->sourceBuffer1 != NULL){
		b->sourceBuffer1->refCounter --;
		if(b->sourceBuffer1->refCounter <= 0){
			freeBuffer(b->sourceBuffer1);
		}
	}
	if(b->sourceBuffer2 != NULL){
		b->sourceBuffer2->refCounter --;
		if(b->sourceBuffer2->refCounter <= 0){
			freeBuffer(b->sourceBuffer2);
		}
	}
	
	if(b == NULL)
		return;
	
	if(b->free != NULL){
		b->free(b);
	}
	free(b);
}



Element *dupElement(Element *elm){
	if(elm == NULL){
		return NULL;
	}
	Element *ret = newElement(elm->type, NULL);
	if(ret->type == ET_STRING){
		ret->data = (void*)dup((char*)elm->data);
	}else if(ret->type == ET_NUMBER ){
		ret->dval = elm->dval;
	}else if(ret->type == ET_VECTOR){
		Vector *v = (Vector*)elm->data;
		Vector *vnew = newVector();
		for(int i=0; i<v->len; ++i){
			vectorPushBack(vnew, dupElement(v->data[i]));
		}
		ret->data = (void*)vnew;
	}else if(ret->type == ET_BUFFER){
		Buffer *b = (Buffer*)elm->data;
		ret->data = (void*)dupBuffer(b);
	}
	
	cloneOps(ret, elm);
	return ret;
}

Token *mqlProc_Elm(State *s, Token *tk){
	Element *elm = NULL;
	
	if(tk->type == TT_NUMBER){
		double x = strtod(tk->s, NULL);
		elm = newElement(ET_NUMBER, NULL);
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
	}else if(elm->type == ET_NUMBER){
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
	}else if(elm->type == ET_BUFFER){
		printf(" [ BUFFER ] \n");
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


