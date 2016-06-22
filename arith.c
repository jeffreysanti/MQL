
#include "methods.h"

char *asString(Element *elm, int *needtoFree){
	*needtoFree = 0;
	if(elm->type == ET_STRING){
		return (char*)elm->data;
	}
	if(elm->type == ET_INTEGER){
		char *str = malloc(24);
		snprintf(str, 24, "%ld", elm->ival);
		*needtoFree = 1;
		return str;
	}
	if(elm->type == ET_DECIMAL){
		char *str = malloc(24);
		snprintf(str, 24, "%f", elm->dval);
		*needtoFree = 1;
		return str;
	}
	return "";
}

double asDouble(Element *elm){
	if(elm->type == TT_INT){
		return (double)elm->ival;
	}
	if(elm->type == TT_FLOAT){
		return elm->dval;
	}
	return 0.0;
}

Element *addTwoElements(Element *op1, Element *op2, State *s){
	Element *push = NULL;
	if(s->invalid || op1==NULL || op2==NULL){
		return NULL;
	}
	
	if(op1->type == ET_VECTOR && op2->type == ET_VECTOR){
		Vector *v1 = (Vector*)op1->data;
		Vector *v2 = (Vector*)op2->data;
		if(v1->len != v2->len){
			s->invalid = 1;
			s->errStr = dup("+ Cannot add unlike lengthed vectors");
			return NULL;
		}
		
		Vector *vnew = newVector();
		for(int i=0; i<v1->len; ++i){
			vectorPushBack(vnew, addTwoElements(v1->data[i], v2->data[i], s));
			if(s->invalid){
				break;
			}
		}
		if(s->invalid){
			freeVector(vnew);
			return NULL;
		}
		
		push = newElement(ET_VECTOR, vnew);
	}else if(op1->type == ET_VECTOR || op2->type == ET_VECTOR){
		Element *eVec = (op1->type == ET_VECTOR) ? op1 : op2;
		Element *eBasic = (op1->type != ET_VECTOR) ? op1 : op2;
		Vector *v = (Vector*)eVec->data;
		Vector *vnew = newVector();
		for(int i=0; i<v->len; ++i){
			vectorPushBack(vnew, addTwoElements(eBasic, v->data[i], s));
			if(s->invalid){
				break;
			}
		}
		if(s->invalid){
			freeVector(vnew);
			return NULL;
		}
		
		push = newElement(ET_VECTOR, vnew);
	}else if(op1->type == ET_STRING || op2->type == ET_STRING){
		int s1Dup = 0;
		int s2Dup = 0;
		char *s1 = asString(op1, &s1Dup);
		char *s2 = asString(op2, &s2Dup);
		
		char *dest = malloc(strlen(s1) + strlen(s2) + 1);
		strcpy(dest, s1);
		char *res = strcat(dest, s2);
		if(s1Dup) free(s1);
		if(s2Dup) free(s2);
		
		push = newElement(ET_STRING, res);
	}
	else if(op1->type == ET_DECIMAL || op2->type == ET_DECIMAL){
		double d1 = asDouble(op1);
		double d2 = asDouble(op2);
		double res = d1 + d2;
		push = newElement(ET_DECIMAL, NULL);
		push->dval = res;
	}else{ // both integers
		long int i1 = op1->ival;
		long int i2 = op2->ival;
		long int res = i1 + i2;
		push = newElement(ET_INTEGER, NULL);
		push->ival = res;
	}
	cloneOps(push, op1);
	cloneOps(push, op2);
	return push;
}

Token* opPlus(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op2 == NULL){
		if(op2 != NULL) freeElement(op2);
		return tk;
	}
	
	Element *push = addTwoElements(op1, op2, s);
	if(s->invalid){
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	// push result
	s->stack = stackPush(s->stack, push);
	
	freeElement(op1);
	freeElement(op2);
	return tk;
}

Element *negateElement(Element *op1, State *s){
	Element *push = NULL;
	if(s->invalid || op1==NULL){
		return NULL;
	}
	
	if(op1->type == ET_VECTOR){
		Vector *v = (Vector*)op1->data;
		Vector *vnew = newVector();
		for(int i=0; i<v->len; ++i){
			vectorPushBack(vnew, negateElement(v->data[i], s));
			if(s->invalid){
				break;
			}
		}
		if(s->invalid){
			freeVector(vnew);
			return NULL;
		}
		
		push = newElement(ET_VECTOR, vnew);
	}else if(op1->type == ET_STRING){
		char *dest = dup((char*)op1->data);
		dest = strrev(dest);
		push = newElement(ET_STRING, dest);
	}else if(op1->type == ET_DECIMAL){
		double d1 = op1->dval;
		double res = -1*d1;
		push = newElement(ET_DECIMAL, NULL);
		push->dval = res;
	}else{ // integer
		long int i1 = op1->ival;
		long int res = -1 * i1;
		push = newElement(ET_INTEGER, NULL);
		push->ival = res;
	}
	cloneOps(push, op1);
	return push;
}

Token* opNeg(State* s, Token* tk){
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL){
		return tk;
	}
	
	Element *push = negateElement(op1, s);
	if(s->invalid){
		freeElement(op1);
		return tk;
	}
	
	// push result
	s->stack = stackPush(s->stack, push);
	
	freeElement(op1);
	return tk;
}

Token* opSub(State* s, Token* tk){
	tk = opNeg(s, tk);
	if(!s->invalid){
		tk = opPlus(s, tk);
	}
	return tk;
}

Element *operateTwoElements(Element *op1, Element *op2, State *s, char op){
	Element *push = NULL;
	if(s->invalid || op1==NULL || op2==NULL){
		return NULL;
	}
	
	if(op1->type == ET_VECTOR && op2->type == ET_VECTOR){
		s->invalid = 1;
		s->errStr = dup("*/% Cannot work on two vectors");
		return NULL;
	}else if(op1->type == ET_STRING || op2->type == ET_STRING){
		s->invalid = 1;
		s->errStr = dup("*/% Cannot work on strings");
		return NULL;
	}else if(op1->type == ET_VECTOR || op2->type == ET_VECTOR){
		Element *eVec = (op1->type == ET_VECTOR) ? op1 : op2;
		Element *eBasic = (op1->type != ET_VECTOR) ? op1 : op2;
		Vector *v = (Vector*)eVec->data;
		Vector *vnew = newVector();
		for(int i=0; i<v->len; ++i){
			vectorPushBack(vnew, operateTwoElements(eBasic, v->data[i], s, op));
			if(s->invalid){
				break;
			}
		}
		if(s->invalid){
			freeVector(vnew);
			return NULL;
		}
		
		push = newElement(ET_VECTOR, vnew);
	}else if(op1->type == ET_DECIMAL || op2->type == ET_DECIMAL){
		double d1 = asDouble(op1);
		double d2 = asDouble(op2);
		double res = 0;
		if(op == '*') res = d1 * d2;
		if(op == '/') res = d1 / d2;
		if(op == '%') res = fmod(d1, d2);
		push = newElement(ET_DECIMAL, NULL);
		push->dval = res;
	}else{ // both integers
		long int i1 = op1->ival;
		long int i2 = op2->ival;
		long int res = 0;
		if(op == '*') res = i1 * i2;
		if(op == '/') res = i1 / i2;
		if(op == '%') res = i1 % i2;
		push = newElement(ET_INTEGER, NULL);
		push->ival = res;
	}
	cloneOps(push, op1);
	cloneOps(push, op2);
	return push;
}

Element *inequality(Element *op1, Element *op2, State *s, char op){
	Element *push = NULL;
	if(s->invalid || op1==NULL || op2==NULL){
		return NULL;
	}
	
	if(op1->type == ET_VECTOR || op2->type == ET_VECTOR){
		s->invalid = 1;
		s->errStr = dup("inequality Cannot work on vectors");
		return NULL;
	}else if(op1->type == ET_STRING || op2->type == ET_STRING){
		char *s1 = (char*)op1->data;
		char *s2 = (char*)op2->data;
		long int res = 0;
		if(op == '>') res = (strcasecmp(s1, s2) > 0) ? 1 : 0;
		if(op == '}') res = (strcasecmp(s1, s2) >= 0) ? 1 : 0;
		if(op == '<') res = (strcasecmp(s1, s2) < 0) ? 1 : 0;
		if(op == '{') res = (strcasecmp(s1, s2) <= 0) ? 1 : 0;
		push = newElement(ET_INTEGER, NULL);
		push->ival = res;
	}else if(op1->type == ET_DECIMAL || op2->type == ET_DECIMAL){
		double d1 = asDouble(op1);
		double d2 = asDouble(op2);
		long int res = 0;
		if(op == '>') res = (d1 > d2) ? 1 : 0;
		if(op == '}') res = (d1 >= d2) ? 1 : 0;
		if(op == '<') res = (d1 < d2) ? 1 : 0;
		if(op == '{') res = (d1 <= d2) ? 1 : 0;
		push = newElement(ET_INTEGER, NULL);
		push->ival = res;
	}else{ // both integers
		long int i1 = op1->ival;
		long int i2 = op2->ival;
		long int res = 0;
		if(op == '>') res = (i1 > i2) ? 1 : 0;
		if(op == '}') res = (i1 >= i2) ? 1 : 0;
		if(op == '<') res = (i1 < i2) ? 1 : 0;
		if(op == '{') res = (i1 <= i2) ? 1 : 0;
		push = newElement(ET_INTEGER, NULL);
		push->ival = res;
	}
	cloneOps(push, op1);
	cloneOps(push, op2);
	return push;
}


Token* opMult(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op2 == NULL){
		if(op2 != NULL) freeElement(op2);
		return tk;
	}
	
	Element *push = operateTwoElements(op1, op2, s, '*');
	if(s->invalid){
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	// push result
	s->stack = stackPush(s->stack, push);
	
	freeElement(op1);
	freeElement(op2);
	return tk;
}

Token* opDiv(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op2 == NULL){
		if(op2 != NULL) freeElement(op2);
		return tk;
	}
	
	Element *push = operateTwoElements(op1, op2, s, '/');
	if(s->invalid){
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	// push result
	s->stack = stackPush(s->stack, push);
	
	freeElement(op1);
	freeElement(op2);
	return tk;
}

Token* opMod(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op2 == NULL){
		if(op2 != NULL) freeElement(op2);
		return tk;
	}
	
	Element *push = operateTwoElements(op1, op2, s, '%');
	if(s->invalid){
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	// push result
	s->stack = stackPush(s->stack, push);
	
	freeElement(op1);
	freeElement(op2);
	return tk;
}

Token* opGT(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op2 == NULL){
		if(op2 != NULL) freeElement(op2);
		return tk;
	}
	
	Element *push = inequality(op1, op2, s, '>');
	if(s->invalid){
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	// push result
	s->stack = stackPush(s->stack, push);
	
	freeElement(op1);
	freeElement(op2);
	return tk;
}

Token* opGTE(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op2 == NULL){
		if(op2 != NULL) freeElement(op2);
		return tk;
	}
	
	Element *push = inequality(op1, op2, s, '}');
	if(s->invalid){
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	// push result
	s->stack = stackPush(s->stack, push);
	
	freeElement(op1);
	freeElement(op2);
	return tk;
}

Token* opLT(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op2 == NULL){
		if(op2 != NULL) freeElement(op2);
		return tk;
	}
	
	Element *push = inequality(op1, op2, s, '<');
	if(s->invalid){
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	// push result
	s->stack = stackPush(s->stack, push);
	
	freeElement(op1);
	freeElement(op2);
	return tk;
}

Token* opLTE(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op2 == NULL){
		if(op2 != NULL) freeElement(op2);
		return tk;
	}
	
	Element *push = inequality(op1, op2, s, '{');
	if(s->invalid){
		freeElement(op1);
		freeElement(op2);
		return tk;
	}
	
	// push result
	s->stack = stackPush(s->stack, push);
	
	freeElement(op1);
	freeElement(op2);
	return tk;
}

Token* opNot(State* s, Token* tk){
	long int res = isTrue(stackPoll(s->stack)) ? 0 : 1;
	Element *push = newElement(ET_INTEGER, NULL);
	push->ival = res;
	
	if(stackPoll(s->stack) != NULL){
		s->stack = stackPop(s->stack);
	}
	s->stack = stackPush(s->stack, push);
	return tk;
}

int equality(Element *e1, Element *e2){
	if(e1 == NULL || e2 == NULL){
		return 0;
	}
	if(e1->type != e2->type){
		return 0;
	}
	if(e1->type == ET_INTEGER){
		return e1->ival == e2->ival;
	}
	if(e1->type == ET_DECIMAL){
		return e1->dval == e2->dval;
	}
	if(e1->type == ET_STRING){
		return strcmp((char*)e1->data, (char*)e2->data) == 0;
	}
	Vector *v1 = (Vector*)e1->data;
	Vector *v2 = (Vector*)e2->data;
	if(v1->len != v2->len){
		return 0;
	}
	for(int i=0; i<v1->len; ++i){
		if(!equality(v1->data[i], v2->data[i])){
			return 0;
		}
	}
	return 1;
}

Token* opEqual(State* s, Token* tk){
	Element *op2 = popStackOrErr(s);
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op2 == NULL){
		if(op2 != NULL) freeElement(op2);
		return tk;
	}
	
	long int res = equality(op1, op2);
	Element *push = newElement(ET_INTEGER, NULL);
	push->ival = res;
	
	// push result
	s->stack = stackPush(s->stack, push);
	
	freeElement(op1);
	freeElement(op2);
	return tk;
}

void registerArithmeticOps(){
	registerGloablOp("+", &opPlus);
	registerGloablOp("neg", &opNeg);
	registerGloablOp("NEG", &opNeg);
	registerGloablOp("-", &opSub);
	registerGloablOp("*", &opMult);
	registerGloablOp("/", &opDiv);
	registerGloablOp("%", &opMod);
	registerGloablOp(">", &opGT);
	registerGloablOp(">=", &opGTE);
	registerGloablOp("<", &opLT);
	registerGloablOp("<=", &opLTE);
	registerGloablOp("not", &opNot);
	registerGloablOp("NOT", &opNot);
	registerGloablOp("=", &opEqual);
}
