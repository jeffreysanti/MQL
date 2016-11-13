#include "methods.h"


typedef struct BufferOfVec BufferOfVec;
struct BufferOfVec {
	Vector *v;
	unsigned int nextIndex;
};

void bufferNextOfVec(State *s, Buffer *b){
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
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op1->type != ET_BUFFER){
		s->invalid = 1;
		s->errStr = dup("Vectorize must take a buffer as input");
		if(op1 != NULL) freeElement(op1);
		return tk;
	}
	
	Buffer *buf = (Buffer*)op1->data;
	Element *velm = newElement(ET_VECTOR, (void*)vectorFromBuffer(s, buf));
	
	freeElement(op1);
	s->stack = stackPush(s->stack, velm);
	return tk;
}

Token* opBufferize(State* s, Token* tk){
	Element *op1 = popStackOrErr(s);
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


Vector *vectorFromBuffer(State *s, Buffer *buf){
	Vector *ret = newVector();
	
	Element *e = NULL;
	while(e = advanceBuffer(s, buf)){
		vectorPushBack(ret, e);
	}
	return ret;
}

Element *advanceBuffer(State *s, Buffer *buf){
	if(buf == NULL){
		return NULL;
	}
	advanceBuffer(s, buf->sourceBuffer1);
	if(buf->sourceBuffer2 != NULL){
		buf->sourceBuffer2->next(s, buf->sourceBuffer2);
	}
	buf->next(s, buf);
	buf->syncCounter ++;
	return buf->lastData;
}

Element *getBufferData(State *s, Buffer *buf){
	if(buf == NULL){
		return NULL;
	}
	if(buf->firstBuffer == NULL){
		return buf->lastData;
	}
	if(buf->syncCounter == buf->firstBuffer->syncCounter){
		return buf->lastData;
	}
	getBufferData(s, buf->sourceBuffer1);
	getBufferData(s, buf->sourceBuffer2);
	while(buf->syncCounter != buf->firstBuffer->syncCounter){
		buf->next(s, buf);
		buf->syncCounter ++;
	}
	return buf->lastData;
}

void bufferNextDup(State *s, Buffer *b){
	if(b->lastData != NULL){
		freeElement(b->lastData);
	}
	b->lastData = dupElement(getBufferData(s, b->sourceBuffer1));
	b->eob = b->sourceBuffer1->eob;
}

Buffer *dupBuffer(Buffer *buf){
	Buffer *b = newBufferWithSource(buf, NULL);
	b->next = &bufferNextDup;
	return b;
}

int commonBufferLineage(Buffer *b1, Buffer *b2){
	if(b1->firstBuffer == NULL && b2->firstBuffer == NULL){
		return 0;
	}
	if(b1->firstBuffer == b2 || b2->firstBuffer == b1){
		return 1;
	}
	return b1->firstBuffer == b2->firstBuffer;
}




typedef struct BufferOfMux BufferOfMux;
struct BufferOfMux {
	Buffer *b1;
	Buffer *b2;
};

void bufferNextOfMux(State *s, Buffer *b){
	BufferOfMux *data = (BufferOfMux*)b->extra;
	
	if(commonBufferLineage(data->b1, data->b2)){
		advanceBuffer(s, data->b1);
	}else{
		advanceBuffer(s, data->b1);
		advanceBuffer(s, data->b2);
	}
	
	Element *op1 = getBufferData(s, data->b1);
	Element *op2 = getBufferData(s, data->b2);
	if(data->b1->eob || data->b2->eob){
		b->eob = 1;
		b->lastData = NULL;
		return;
	}
	
	b->lastData = mergeNoDuplicateVectors(op1, op2);
}
void bufferFreeOfMux(Buffer *b){
	if(b->extra != NULL){
		BufferOfMux *data = (BufferOfMux*)b->extra;
		freeBuffer(data->b1);
		freeBuffer(data->b2);
		free(data);
		b->extra = NULL;
	}
}


Token* opMultiplex(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL){
		return tk;
	}
	
	if(op1->type != ET_BUFFER || op2->type != ET_BUFFER){
		s->invalid = 1;
		s->errStr = dup("Mux must take two buffers");
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	BufferOfMux *data = malloc(sizeof(BufferOfMux));
	data->b1 = (Buffer*)op1->data;
	data->b2 = (Buffer*)op2->data;
	data->b1->refCounter ++;
	data->b2->refCounter ++;
	
	Buffer *b = newBufferOriginal();
	b->next = &bufferNextOfMux;
	b->free = &bufferFreeOfMux;
	b->extra = data;
	
	Element *belm = newElement(ET_BUFFER, (void*)b);
	cloneOps(belm, op1);
	cloneOps(belm, op2);
	
	freeElement(op1);
	freeElement(op2);
	s->stack = stackPush(s->stack, belm);
	return tk;
}


typedef struct BufferConstant BufferConstant;
struct BufferConstant {
	Element *repeat;
};

void bufferNextConstant(State *s, Buffer *b){
	BufferConstant *data = (BufferConstant*)b->extra;
	
	b->lastData = dupElement(data->repeat);
}
void bufferFreeConstant(Buffer *b){
	if(b->extra != NULL){
		BufferConstant *data = (BufferConstant*)b->extra;
		freeElement(data->repeat);
		free(data);
		b->extra = NULL;
	}
}

Element *constantBuffer(Element *elm){
	BufferConstant *data = malloc(sizeof(BufferConstant));
	data->repeat = elm;
	
	Buffer *b = newBufferOriginal();
	b->next = &bufferNextConstant;
	b->free = &bufferFreeConstant;
	b->extra = data;
	
	Element *belm = newElement(ET_BUFFER, (void*)b);
	return belm;
}

void registerBufferOps(){
	registerGloablOp("->v", &opVectorize);
	registerGloablOp("->b", &opBufferize);
	registerGloablOp("mux", &opMultiplex);
	registerGloablOp("MUX", &opMultiplex);
}


