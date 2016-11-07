#include "methods.h"


typedef struct BufferOfVec BufferOfVec;
struct BufferOfVec {
	Vector *v;
	unsigned int nextIndex;
};

void bufferNextOfVec(Buffer *b){
	BufferOfVec *data = (BufferOfVec*)b->extra;
	if(data->nextIndex >= data->v->len){
		b->eob = 1;
		b->lastData = NULL;
		return;
	}
	b->lastData = data->v->data[data->nextIndex];
	data->v->data[data->nextIndex] = NULL;
	++ data->nextIndex;
}
void bufferFreeOfVec(Buffer *b){
	if(b->extra != NULL){
		BufferOfVec *data = (BufferOfVec*)b->extra;
		freeVector(data->v);
		free(data);
		b->extra = NULL;
	}
}

Buffer *bufferFromVector(Vector *vec){
	BufferOfVec *data = malloc(sizeof(BufferOfVec));
	data->v = vec;
	data->nextIndex = 0;
	
	Buffer *b = newBufferOriginal();
	b->next = &bufferNextOfVec;
	b->free = &bufferFreeOfVec;
	b->extra = data;
	return b;
}

Token* opVectorize(State* s, Token* tk){
	Element *op1 = stackPoll(s->stack);
	if(op1 == NULL || op1->type != ET_BUFFER){
		s->invalid = 1;
		s->errStr = dup("Vectorize must take a buffer as input");
		if(op1 != NULL) freeElement(op1);
		return tk;
	}
	
	Buffer *buf = (Buffer*)op1->data;
	Element *velm = newElement(ET_VECTOR, (void*)vectorFromBuffer(buf));
	
	freeElement(op1);
	s->stack = stackPush(s->stack, velm);
	return tk;
}

Token* opBufferize(State* s, Token* tk){
	Element *op1 = stackPoll(s->stack);
	if(op1 == NULL || op1->type != ET_VECTOR){
		s->invalid = 1;
		s->errStr = dup("Bufferize must take a vector as input");
		if(op1 != NULL) freeElement(op1);
		return tk;
	}
	
	Vector *vec = (Vector*)op1->data;
	Element *belm = newElement(ET_BUFFER, (void*)bufferFromVector(vec));
	
	freeElement(op1);
	s->stack = stackPush(s->stack, belm);
	return tk;
}


Vector *vectorFromBuffer(Buffer *buf){
	Vector *ret = newVector();
	
	Element *e = NULL;
	while(e = advanceBuffer(buf)){
		vectorPushBack(ret, e);
	}
	return ret;
}

Element *advanceBuffer(Buffer *buf){
	if(buf == NULL){
		return NULL;
	}
	advanceBuffer(buf->sourceBuffer1);
	if(buf->sourceBuffer2 != NULL){
		buf->sourceBuffer2->next(buf->sourceBuffer2);
	}
	buf->next(buf);
	buf->syncCounter ++;
	return buf->lastData;
}

Element *getBufferData(Buffer *buf){
	if(buf == NULL){
		return NULL;
	}
	if(buf->firstBuffer == NULL){
		return buf->lastData;
	}
	if(buf->syncCounter == buf->firstBuffer->syncCounter){
		return buf->lastData;
	}
	getBufferData(buf->sourceBuffer1);
	getBufferData(buf->sourceBuffer2);
	while(buf->syncCounter != buf->firstBuffer->syncCounter){
		buf->next(buf);
		buf->syncCounter ++;
	}
	return buf->lastData;
}

void registerBufferOps(){
	registerGloablOp("->v", &opVectorize);
	registerGloablOp("->b", &opBufferize);
}


