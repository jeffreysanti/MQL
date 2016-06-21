#include "methods.h"

int isSpecialCase(Token *tk){
	if(tk->type != TT_OP){
		return 0;
	}
	return (!strcmp(tk->s, "PUSH") || !strcmp(tk->s, "push") || 
				!strcmp(tk->s, "CONTINUE") || !strcmp(tk->s, "continue"));
}

int isTrue(Element *elm){
	if(elm == NULL || elm->data == NULL){
		return 0;
	}
	if(elm->type == ET_INTEGER){
		long int val = *(long int*)elm->data;
		return (val == 0) ? 0 : 1;
	}else if(elm->type == ET_DECIMAL){
		double val = *(double*)elm->data;
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

Token* opNop(State* s, Token* tk){
	return tk;
}


Token* opIf(State* s, Token* tk){
	int branchTrue = isTrue(stackPoll(s->stack));
	if(stackPoll(s->stack) != NULL){
		s->stack = stackPop(s->stack);
	}

	if(!branchTrue){
		int level = 0;
		while(tk != NULL){
			if(tk->type == TT_OP && (!strcmp(tk->s, "IF") || !strcmp(tk->s, "if"))){
				++level;
			}
			if(tk->type == TT_OP && level == 0 && (!strcmp(tk->s, "ELSE") || !strcmp(tk->s, "else"))){
				tk = tk->next;
				break;
			}
			if(tk->type == TT_OP && (!strcmp(tk->s, "FI") || !strcmp(tk->s, "fi"))){
				if(level == 0){
					return tk->next; // entire branch skipped
				}
				--level;
			}
			tk = tk->next;
		}
	}
	
	// now execute until we reach the else or fi 
	while(tk != NULL){
		if(s->invalid)
			return tk;
		
		if(tk->type == TT_OP && (!strcmp(tk->s, "FI") || !strcmp(tk->s, "fi"))){
			return tk->next;
		}
		if(tk->type == TT_OP && (!strcmp(tk->s, "ELSE") || !strcmp(tk->s, "else"))){
			tk = tk->next;
			int level = 0;
			while(tk != NULL){
				if(tk->type == TT_OP && (!strcmp(tk->s, "IF") || !strcmp(tk->s, "if"))){
					++level;
				}
				if(tk->type == TT_OP && (!strcmp(tk->s, "FI") || !strcmp(tk->s, "fi"))){
					if(level == 0){
						return tk->next;
					}
					--level;
				}
				tk = tk->next;
			}
		}
		
		if(tk == NULL){
			return tk;
		}
		
		if(isSpecialCase(tk)){
			return tk;
		}
		if(tk->type == TT_INT || tk->type == TT_FLOAT || tk->type == TT_STRING){
			tk = mqlProc_Elm(s, tk);
		}else if(tk->type == TT_DEFINE){
			tk = mqlProc_Def(s, tk);
		}else{
			tk = mql_op(s, tk);
		}
	}
	return tk;
}

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
}



void registerControlOps(){
	registerGloablOp("IF", &opIf);
	registerGloablOp("if", &opIf);
	registerGloablOp("NOP", &opNop);
	registerGloablOp("nop", &opNop);
	registerGloablOp("FOR", &opFor);
	registerGloablOp("for", &opFor);
}

