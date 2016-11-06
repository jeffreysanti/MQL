#include "methods.h"


typedef struct BufferOfVec BufferOfVec;
struct BufferOfVec {
	Vector *v;
	unsigned int nextIndex;
};

Element *bufferNextOfVec(Buffer *b){
	BufferOfVec *data = (BufferOfVec*)b->extra;
	if(data->nextIndex >= data->v->len){
		b->eob = 1;
		return NULL;
	}
	Element *ret = data->v[data->nextIndex];
	data->v[data->nextIndex] = NULL;
	++ data->nextIndex;
	return ret;
}
void bufferFreeOfVec(Buffer *b){
	if(b->extra != NULL){
		BufferOfVec *data = (BufferOfVec*)b->extra;
		freeVector(data->v);
		free(data);
		b->extra = NULL;
	}
}

Element *asBuffer(Element *e){
	if(e->type == ET_BUFFER){
		return e;
	}else if(e->type == ET_VECTOR){
		BufferOfVec *data = malloc(sizeof(BufferOfVec));
		data->v = (Vector*)e->data;
		data->nextIndex = 0;
		
		Buffer *b = newBuffer();
		b->next = &bufferNextOfVec;
		b->free = &bufferFreeOfVec;
		b->extra = data;
		
		e->data = b;
		e->type = ET_BUFFER;
		return e;
	}else{
		Vector *vnew = newVector();
		vectorPushBack(vnew, e);
		Element *enew = newElement(ET_VECTOR, vnew);
		
		return asBuffer(enew);
	}
}
