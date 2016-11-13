#include "methods.h"

int isSpecialCase(Token *tk){
	if(tk->type != TT_OP){
		return 0;
	}
	//return (!strcmp(tk->s, "PUSH") || !strcmp(tk->s, "push") || 
	//			!strcmp(tk->s, "CONTINUE") || !strcmp(tk->s, "continue"));
	return 0;
}

int isTrue(Element *elm){
	if(elm == NULL){
		return 0;
	}
	if(elm->type == ET_NUMBER){
		double val = elm->dval;
		return (val == 0) ? 0 : 1;
	}else if(elm->type == ET_STRING){
		char *s = (char*)elm->data;
		return (strlen(s) == 0) ? 0 : 1;
	}else if(elm->type == ET_VECTOR){
		Vector *v = (Vector*)elm->data;
		return (v->len > 0) ? 1 : 0;
	}
	return 0;
}

Element *trueBuffer(){
	Element *etrue = newElement(ET_NUMBER, NULL);
	etrue->dval = 1;
	return constantBuffer(etrue);
}

Element *falseBuffer(){
	Element *efalse = newElement(ET_NUMBER, NULL);
	efalse->dval = 0;
	return constantBuffer(efalse);
}

Token* opNop(State* s, Token* tk){
	return tk;
}



typedef struct BufferIf BufferIf;
struct BufferIf {
	Token *branchTrue;
	Token *branchFalse;
	Buffer *bcond;
	Buffer *biterate;
};

void bufferNextIf(State* s, Buffer *b){
	while(1){
		BufferIf *data = (BufferIf*)b->extra;
		Element *cond = advanceBuffer(s, data->bcond);
		
		if(data->biterate != NULL){
			if(!commonBufferLineage(data->bcond, data->biterate)){
				advanceBuffer(s, data->biterate);
			}
			
			Element *econd = getBufferData(s, data->bcond);
			Element *eiterate = getBufferData(s, data->biterate);
			if(data->bcond->eob || data->biterate->eob){
				b->eob = 1;
				b->lastData = NULL;
				return;
			}
			
			// Use a clean stack inside conditional code blocks
			Stack *oldStack = s->stack;
			
			s->stack = newStack();
			s->stack = stackPush(s->stack, eiterate);
			int truth = isTrue(cond);
			freeElement(cond);
			if(truth){
				mqlCodeBlock(s, data->branchTrue);
			}else if(data->branchFalse != NULL){
				mqlCodeBlock(s, data->branchFalse);
			}
			b->lastData = stackPoll(s->stack);
			s->stack = stackPopNoFree(s->stack);
			s->stack = stackPopAll(s->stack);
			
			// restore old stack
			s->stack = oldStack;
		}else{
			Element *econd = getBufferData(s, data->bcond);
			if(data->bcond->eob){
				b->eob = 1;
				b->lastData = NULL;
				return;
			}
			
			// Use a clean stack inside conditional code blocks
			Stack *oldStack = s->stack;
			
			s->stack = newStack();
			int truth = isTrue(cond);
			freeElement(cond);
			if(truth){
				mqlCodeBlock(s, data->branchTrue);
			}else if(data->branchFalse != NULL){
				mqlCodeBlock(s, data->branchFalse);
			}
			b->lastData = stackPoll(s->stack);
			s->stack = stackPopNoFree(s->stack);
			s->stack = stackPopAll(s->stack);
			
			// restore old stack
			s->stack = oldStack;
		}
		
		if(b->lastData != NULL){ // a successful push to buffer
			break;
		}
	}
}
void bufferFreeIf(Buffer *b){
	if(b->extra != NULL){
		BufferIf *data = (BufferIf*)b->extra;
		freeBuffer(data->bcond);
		if(data->biterate != NULL)
			freeBuffer(data->biterate);
		freeToken(data->branchTrue);
		if(data->branchFalse != NULL)
			freeToken(data->branchFalse);
		free(data);
		b->extra = NULL;
	}
}

Token* opIf(State* s, Token* tk){
	Element *cond = stackPoll(s->stack);
	if(cond != NULL){
		s->stack = stackPopNoFree(s->stack);
	}
	
	Token *branchTrue = tk;
	Token *branchCont = branchTrue->next;
	Token *branchFalse = NULL;
	
	if(branchCont != NULL && branchCont->next != NULL && branchCont->type == TT_OP && (!strcmp(branchCont->s, "ELSE") || !strcmp(branchCont->s, "else"))){
		branchFalse = branchCont->next;
		branchCont = branchCont->next->next;
	}
	
	if(cond != NULL && cond->type == ET_BUFFER){
		BufferIf *data = malloc(sizeof(BufferIf));
		data->branchTrue = duplicateToken(branchTrue);
		data->branchFalse = NULL;
		if(branchFalse != NULL)
			data->branchFalse = duplicateToken(branchFalse);
		data->bcond = (Buffer*)cond->data;
		data->bcond->refCounter ++;
		data->biterate = NULL;
		
		Element *iterateOp = stackPoll(s->stack);
		if(iterateOp != NULL && iterateOp->type == ET_BUFFER){
			data->biterate = (Buffer*)iterateOp->data;
			data->biterate->refCounter ++;
		}
		
		Buffer *b = newBufferOriginal();
		b->next = &bufferNextIf;
		b->free = &bufferFreeIf;
		b->extra = data;
		
		Element *belm = newElement(ET_BUFFER, (void*)b);
		cloneOps(belm, cond);
		if(iterateOp != NULL){
			cloneOps(belm, iterateOp);
			s->stack = stackPop(s->stack);
		}
		
		freeElement(cond);
		s->stack = stackPush(s->stack, belm);
	}else if(stackPoll(s->stack) != NULL && stackPoll(s->stack)->type == ET_BUFFER){ // single condition loop
		int truth = isTrue(cond);
		freeElement(cond);
		
		// add a constant buffer of truth or falseness
		if(truth){
			s->stack = stackPush(s->stack, trueBuffer());
		}else{
			s->stack = stackPush(s->stack, falseBuffer());
		}
		
		// now try this if again
		return opIf(s, tk);
	}else{
		int truth = isTrue(cond);
		freeElement(cond);
		if(truth){
			mqlCodeBlock(s, branchTrue);
		}else if(branchFalse != NULL){
			mqlCodeBlock(s, branchFalse);
		}
	}
	
	return branchCont;
}

Token* opFor(State* s, Token* tk){
	Element *op1 = stackPoll(s->stack);
	if(op1 == NULL || op1->type != ET_BUFFER){
		s->invalid = 1;
		s->errStr = dup("For can only operate on a buffer");
		freeElement(op1);
		return tk;
	}
	
	// now push an eternal truth and treat it like an if statement
	s->stack = stackPush(s->stack, trueBuffer());
	return opIf(s, tk);
}

/*
Token* opFor(State* s, Token* tk){
	autoPackStack(s);
	Element *top = stackPoll(s->stack);
	Vector *v = (Vector*)top->data;
	s->stack = stackPopNoFree(s->stack);
	
	Vector *vnew = newVector();
	Element *enew = newElement(ET_VECTOR, vnew);
	
	
	Token *start = tk;
	for(int i=0; i<v->len; ++i){
		tk = start;
		s->stack = stackPush(s->stack, dupElement(v->data[i]));
		
		while(tk != NULL){
			if(s->invalid){
				freeElement(top);
				freeElement(enew);
				return tk;
			}
			
			if(tk->type == TT_OP && (!strcmp(tk->s, "ORF") || !strcmp(tk->s, "orf"))){
				break;
			}else if(tk->type == TT_OP && (!strcmp(tk->s, "CONTINUE") || !strcmp(tk->s, "continue"))){
				s->stack = stackPop(s->stack);
				break;
			}
			if(tk->type == TT_OP && (!strcmp(tk->s, "PUSH") || !strcmp(tk->s, "push"))){
				Element *push = stackPoll(s->stack);
				s->stack = stackPopNoFree(s->stack);
				vectorPushBack(vnew, push);
				break;
			}
			
			if(tk->type == TT_INT || tk->type == TT_FLOAT || tk->type == TT_STRING){
				tk = mqlProc_Elm(s, tk);
			}else if(tk->type == TT_DEFINE){
				tk = mqlProc_Def(s, tk);
			}else{
				tk = mql_op(s, tk);
			}
		}
		s->stack = stackPop(s->stack);
	}
	cloneOps(enew, top);
	freeElement(top);
	s->stack = stackPush(s->stack, enew);
	
	tk = start;
	int level = 0;
	while(tk != NULL){
		if(tk->type == TT_OP && (!strcmp(tk->s, "FOR") || !strcmp(tk->s, "for"))){
			++level;
		}
		if(tk->type == TT_OP && (!strcmp(tk->s, "ORF") || !strcmp(tk->s, "orf"))){
			if(level == 0){
				return tk->next; // end of for loop
			}
			--level;
		}
		tk = tk->next;
	}
	return tk;
}*/




void registerControlOps(){
	registerGloablOp("IF", &opIf);
	registerGloablOp("if", &opIf);
	
	registerGloablOp("FOR", &opFor);
	registerGloablOp("for", &opFor);
	
	registerGloablOp("NOP", &opNop);
	registerGloablOp("nop", &opNop);
	//registerGloablOp("FOR", &opFor);
	//registerGloablOp("for", &opFor);
}

