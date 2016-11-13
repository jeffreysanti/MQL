
#include "methods.h"
#include <math.h>

#define BUFARITHOPS_B_B 1
#define BUFARITHOPS_N_B 2
#define BUFARITHOPS_B_N 3
#define BUFARITHOPS_B   4


#define TWO_OP_ARITH_FUNC(FNAME, OP) \
Token* FNAME (State* s, Token* tk){ \
	Element *op2 = popStackOrErr(s); \
	Element *op1 = popStackOrErr(s); \
	if(op1 == NULL || op2 == NULL){ \
		if(op2 != NULL) freeElement(op2); \
		return tk; \
	} \
	Element *push = processTwoOperand(s, op1, op2, OP ); \
	s->stack = stackPush(s->stack, push); \
	freeElement(op1); \
	freeElement(op2); \
	return tk; \
}

#define SINGLE_OP_ARITH_FUNC(FNAME, OP) \
Token* FNAME (State* s, Token* tk){ \
	Element *op1 = popStackOrErr(s); \
	if(op1 == NULL){ \
		return tk; \
	} \
	Element *push = processOneOperand(s, op1, OP ); \
	s->stack = stackPush(s->stack, push); \
	freeElement(op1); \
	return tk; \
}



char *asString(Element *elm, int *needtoFree){
	*needtoFree = 0;
	if(elm->type == ET_STRING){
		return (char*)elm->data;
	}
	if(elm->type == ET_NUMBER){
		char *str = malloc(24);
		snprintf(str, 24, "%f", elm->dval);
		*needtoFree = 1;
		return str;
	}
	return "";
}

double asDouble(Element *elm){
	if(elm->type == TT_NUMBER){
		return elm->dval;
	}
	return 0.0;
}

typedef struct BufferArith BufferArith;
struct BufferArith {
	char opArangement;
	char op;
	Element *otherValue;
};

Element *genericArith(Element *op1, Element *op2, char op, int *err){
	double dop1 = 0;
	double dop2 = 0;
	double dres = 0;
	Vector *vop1 = NULL;
	Vector *vop2 = NULL;
	Vector *vres = NULL;
	char *cop1 = NULL;
	char *cop2 = NULL;
	char *cres = NULL;
	
	
	if(op1->type == ET_NUMBER) dop1 = op1->dval;
	if(op1->type == ET_VECTOR) vop1 = (Vector*)op1->data;
	if(op1->type == ET_STRING) cop1 = (char*)op1->data;
	
	if(op2 != NULL){
		if(op2->type == ET_NUMBER) dop2 = op2->dval;
		if(op2->type == ET_VECTOR) vop2 = (Vector*)op2->data;
		if(op2->type == ET_STRING) cop2 = (char*)op2->data;
	}
	
	if((op == '+' || op == '-' || op == '=' || op == '>' || op == '<' || op == '[' || op == ']' || op == 'A' || op == 'O') && 
				vop1 != NULL && vop2 != NULL && vop1->len == vop2->len)
				{ // vector add / sub / (in)equality / andor
		vres = newVector();
		for(int i=0; i<vop1->len; ++i){
			vectorPushBack(vres, genericArith(vop1->data[i], vop2->data[i], op, err));
		}
		if(*err != 0){
			freeVector(vres);
			vres = NULL;
		}
	}else if(op == '+' && (cop1 != NULL || cop2 != NULL)){ // string add
		int s1Dup = 0;
		int s2Dup = 0;
		char *s1 = asString(op1, &s1Dup);
		char *s2 = asString(op2, &s2Dup);
		
		char *dest = malloc(strlen(s1) + strlen(s2) + 1);
		strcpy(dest, s1);
		cres = strcat(dest, s2);
		if(s1Dup) free(s1);
		if(s2Dup) free(s2);
	}else if(op == '+'){ // add
		dres = dop1 + dop2;
	}else if(op == '-'){ // sub
		dres = dop1 - dop2;
	}else if(op == '*' && vop1 != NULL && vop2 == NULL){ // vector scalar multitply
		vres = newVector();
		for(int i=0; i<vop1->len; ++i){
			vectorPushBack(vres, genericArith(vop1->data[i], op2, op, err));
		}
		if(*err != 0){
			freeVector(vres);
			vres = NULL;
		}
	}else if(op == '*' && vop2 != NULL && vop1 == NULL){ // vector scalar multitply
		vres = newVector();
		for(int i=0; i<vop2->len; ++i){
			vectorPushBack(vres, genericArith(vop2->data[i], op1, op, err));
		}
		if(*err != 0){
			freeVector(vres);
			vres = NULL;
		}
	}else if(op == '*'){ // mult
		dres = dop1 * dop2;
	}else if(op == '/'){ // div
		dres = dop1 / dop2;
	}else if(op == '^'){ // exp
		dres = pow(dop1, dop2);
	}else if(op == '>'){
		dres = dop1 > dop2;
	}else if(op == '<'){
		dres = dop1 < dop2;
	}else if(op == '='){
		dres = dop1 == dop2;
	}else if(op == '['){ // <=
		dres = dop1 <= dop2;
	}else if(op == ']'){ // >=
		dres = dop1 >= dop2;
	}else if(op == '!'){
		dres = ! dop1;
	}else if(op == 'N'){
		dres = - dop1;
	}else if(op == 'A'){
		dres = dop1 && dop2;
	}else if(op == 'O'){
		dres = dop1 || dop2;
	}
	
	
	Element *push = NULL;
	if(cres != NULL){
		push = newElement(ET_STRING, cres);
	}else if(vres != NULL){
		push = newElement(ET_VECTOR, vres);
	}else{
		push = newElement(ET_NUMBER, NULL);
		push->dval = dres;
	}
	
	cloneOps(push, op1);
	cloneOps(push, op2);
//	freeElement(op1);
//	freeElement(op2);
	return push;
}


void bufferNextArith(State *s, Buffer *b){
	BufferArith *arith = (BufferArith*)b->extra;
	Element *op1 = NULL;
	Element *op2 = NULL;
	
	if(arith->opArangement == BUFARITHOPS_B_B){
		op1 = getBufferData(s, b->sourceBuffer1);
		op2 = getBufferData(s, b->sourceBuffer2);
		if(b->sourceBuffer1->eob || b->sourceBuffer2->eob){
			b->eob = 1;
			b->lastData = NULL;
			return;
		}
	}else if(arith->opArangement == BUFARITHOPS_B_N){
		op1 = getBufferData(s, b->sourceBuffer1);
		op2 = dupElement(arith->otherValue);
		if(b->sourceBuffer1->eob){
			b->eob = 1;
			b->lastData = NULL;
			return;
		}
	}else if(arith->opArangement == BUFARITHOPS_N_B){
		if(b->sourceBuffer1->eob){
			b->eob = 1;
			b->lastData = NULL;
			return;
		}
		op2 = getBufferData(s, b->sourceBuffer1);
		op1 = dupElement(arith->otherValue);
	}
	
	int err = 0;
	Element *res = genericArith(op1, op2, arith->op, &err);
	b->lastData = res;
}
void bufferFreeArith(Buffer *b){
	if(b->extra != NULL){
		BufferArith *data = (BufferArith*)b->extra;
		freeElement(data->otherValue);
		free(data);
		b->extra = NULL;
	}
}


Element *twoBufferArithOperation(Element *op1, Element *op2, State *s, char op){
	Buffer *b1 = (Buffer*)op1->data;
	Buffer *b2 = (Buffer*)op2->data;
	if(!commonBufferLineage(b1, b2)){
		s->invalid = 1;
		s->errStr = dup("+ Cannot add non-multiplexed buffers");
		return NULL;
	}
	
	BufferArith *data = malloc(sizeof(BufferArith));
	data->opArangement = BUFARITHOPS_B_B;
	data->op = op;
	data->otherValue = 0;
	
	Buffer *b = newBufferWithSource(b1, b2);
	b->next = &bufferNextArith;
	b->free = &bufferFreeArith;
	b->extra = data;
	
	Element *push = newElement(ET_BUFFER, b);
	cloneOps(push, op1);
	cloneOps(push, op2);
	return push;
}

Element *oneBufferArithOperation(Element *op1, Element *op2, State *s, char op){
	Buffer *buf = NULL;
	Element *nonbuf = NULL;
	Element *elmbuf = NULL;
	
	BufferArith *data = malloc(sizeof(BufferArith));
	
	if(op1->type == ET_BUFFER){
		buf = (Buffer*)op1->data;
		nonbuf = op2;
		elmbuf = op1;
		data->opArangement = BUFARITHOPS_B_N;
	}else{
		buf = (Buffer*)op2->data;
		nonbuf = op1;
		elmbuf = op2;
		data->opArangement = BUFARITHOPS_N_B;
	}
	
	data->op = op;
	data->otherValue = dupElement(nonbuf);
	
	Buffer *b = newBufferWithSource(buf, NULL);
	b->next = &bufferNextArith;
	b->free = &bufferFreeArith;
	b->extra = data;
	
	Element *push = newElement(ET_BUFFER, b);
	cloneOps(push, elmbuf);
	return push;
}

Element *processTwoOperand(State *s, Element *op1, Element *op2, char op){
	if(op1->type == ET_BUFFER && op2->type == ET_BUFFER){
		return twoBufferArithOperation(op1, op2, s, op);
	}else if(op1->type == ET_BUFFER || op2->type == ET_BUFFER){
		return oneBufferArithOperation(op1, op2, s, op);
	}else{
		return genericArith(op1, op2, op, &s->invalid);
	}
}

Element *processOneOperand(State *s, Element *op1, char op){
	if(op1->type == ET_BUFFER){
		return oneBufferArithOperation(op1, NULL, s, op);
	}else{
		return genericArith(op1, NULL, op, &s->invalid);
	}
}



TWO_OP_ARITH_FUNC(opPlus, '+')
TWO_OP_ARITH_FUNC(opSub, '-')
TWO_OP_ARITH_FUNC(opMult, '*')
TWO_OP_ARITH_FUNC(opDiv, '/')
TWO_OP_ARITH_FUNC(opExp, '^')

TWO_OP_ARITH_FUNC(opEQ, '=')
TWO_OP_ARITH_FUNC(opGT, '>')
TWO_OP_ARITH_FUNC(opGTE, ']')
TWO_OP_ARITH_FUNC(opLT, '<')
TWO_OP_ARITH_FUNC(opLTE, '[')

TWO_OP_ARITH_FUNC(opAnd, '&')
TWO_OP_ARITH_FUNC(opOr, '|')

SINGLE_OP_ARITH_FUNC(opNot, '!')

SINGLE_OP_ARITH_FUNC(opNeg, 'N')

void registerArithmeticOps(){
	registerGloablOp("+", &opPlus);
	registerGloablOp("-", &opSub);
	registerGloablOp("*", &opMult);
	registerGloablOp("/", &opDiv);
	registerGloablOp("^", &opExp);
	
	registerGloablOp(">", &opGT);
	registerGloablOp(">=", &opGTE);
	registerGloablOp("<", &opLT);
	registerGloablOp("<=", &opLTE);
	registerGloablOp("=", &opEQ);
	
	registerGloablOp("AND", &opAnd);
	registerGloablOp("and", &opAnd);
	registerGloablOp("OR", &opOr);
	registerGloablOp("or", &opOr);
	
	
	registerGloablOp("not", &opNot);
	registerGloablOp("NOT", &opNot);
	
	registerGloablOp("NEG", &opNeg);
	registerGloablOp("net", &opNeg);
	
	/*registerGloablOp("neg", &opNeg);
	registerGloablOp("NEG", &opNeg);
	
	
	
	*/
}
