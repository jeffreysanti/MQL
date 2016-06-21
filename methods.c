#include "methods.h"

Method *M = NULL;
unsigned int MCNT;
unsigned int MALLOC;

Method *newMethod(){
	++MCNT;
	if(MCNT > MALLOC){
		MALLOC = MALLOC * 2;
		M = realloc(M, MALLOC*sizeof(Method));
	}
	Method *m = &M[MCNT-1];
	m->start = NULL;
	m->mid = MCNT-1;
	return m;
}

void freeMethod(Method *m){
	if(m->start != NULL){
		freeToken(m->start);
		m->start = NULL;
	}
}

void initMethodSpace(){
	MALLOC = 2;
	MCNT = 0;
	M = malloc(sizeof(Method) * MALLOC);
}

void freeMethodSpace(){
	for(unsigned int i=0; i<MCNT; ++i){
		freeMethod(&M[i]);
	}
}



void freeMethodListEntry(MethodList **ml, MethodList *mle){
	HASH_DEL(*ml, mle);
	free(mle->name);
	free(mle);
}

void freeMethodList(MethodList **ml){
	MethodList *tmp = NULL, *ent = NULL;
	HASH_ITER(hh, *ml, ent, tmp) {
		freeMethodListEntry(ml, ent);
	}
}

MethodList *newMethodList(){
	return NULL;
}

void addMethod(MethodList **ml, char *s, unsigned int mid){
	s = dup(s);
	MethodList *ent = NULL;
	
	HASH_FIND_STR(*ml, s, ent);
	if(ent != NULL){
		freeMethodListEntry(ml, ent);
		HASH_DEL(*ml, ent);
	}
	ent = malloc(sizeof(MethodList));
	ent->name = s;
	ent->mid = mid;
	HASH_ADD_KEYPTR(hh, *ml, ent->name, strlen(ent->name), ent);
}

void removeMethod(MethodList **ml, char *s){
	MethodList *ent = NULL;
	
	HASH_FIND_STR(*ml, s, ent);
	if(ent != NULL){
		freeMethodListEntry(ml, ent);
		HASH_DEL(*ml, ent);
	}
}

Token *findMethod(MethodList *ml, char *s){
	MethodList *ent = NULL;
	
	HASH_FIND_STR(ml, s, ent);
	if(ent != NULL){
		return M[ent->mid].start;
	}
	return NULL;
}

Element *findSymbol(SymbolTable *st, char *s){
	SymbolTable *ent = NULL;
	
	HASH_FIND_STR(st, s, ent);
	if(ent != NULL){
		return ent->elm;
	}
	return NULL;
}

SymbolTable *newSymbolTable(){
	return NULL;
}

void freeSymbolTableEntry(SymbolTable **st, SymbolTable *ste){
	HASH_DEL(*st, ste);
	free(ste->name);
	freeElement(ste->elm);
	free(ste);
}

void freeSymbolTable(SymbolTable **st){
	SymbolTable *tmp = NULL, *ent = NULL;
	HASH_ITER(hh, *st, ent, tmp) {
		freeSymbolTableEntry(st, ent);
	}
}

void addSymbol(SymbolTable **st, char *s, Element *elm){
	s = dup(s);
	SymbolTable *ent = NULL;
	
	HASH_FIND_STR(*st, s, ent);
	if(ent != NULL){
		freeSymbolTableEntry(st, ent);
	}
	ent = malloc(sizeof(SymbolTable));
	ent->name = s;
	ent->elm = elm;
	HASH_ADD_KEYPTR(hh, *st, ent->name, strlen(ent->name), ent);
}

void cloneOps(Element *dest, Element *src){
	MethodList *tmp = NULL, *ent = NULL;
	HASH_ITER(hh, src->methods, ent, tmp) {
		addMethod(&dest->methods, ent->name, ent->mid);
	}
}

Token* preprocessMethodsAux(State* s, Token* tk, unsigned int level, unsigned int *mid){
	Token *root = tk;
	while(tk != NULL){
		if(tk->type == TT_OP && !strcmp(tk->s, ":")){
			Token *tkName = tk->next;
			if(tkName == NULL || tkName->type != TT_OP){
				s->invalid = 1;
				s->errStr = dup("Define not followed by op name");
				return tk;
			}
			
			tk->type = TT_DEFINE;
			TokenDefine *td = malloc(sizeof(TokenDefine));
			td->s = tkName->s;
			free(tk->s);
			tk->s = (char*)td;
			
			
			tk->next = preprocessMethodsAux(s, tkName->next, level+1, &td->mid);
			
			tkName->next = NULL;
			tkName->s = NULL;
			freeToken(tkName);
		}
		if(level > 0 && tk->next != NULL && tk->next->type == TT_OP && !strcmp(tk->next->s, ";")){
			Token *stopToken = tk->next;
			tk->next = NULL;
			Token *continueToken = stopToken->next;
			stopToken->next = NULL;
			freeToken(stopToken);
			
			Method *method = newMethod();
			method->start = root;
			*mid = method->mid;
			
			return continueToken;
		}
		tk = tk->next;
	}
	
	return root;
}

Token* preprocessMethods(State* s, Token* tk){
	unsigned int na;
	return preprocessMethodsAux(s, tk, 0, &na);
}


Token *mqlProc_Def(State *s, Token *tk){
	TokenDefine *td = (TokenDefine*)tk->s;
	
	Element *op = stackPoll(s->stack);
	if(op == NULL){
		addMethod(&s->globalMethods, td->s, td->mid);
	}else{
		addMethod(&op->methods, td->s, td->mid);
	}
	
	return tk->next;
}

/*
Token* opDef(State* s, Token* tk){
	// first: let's just assume it's global
	if(tk == NULL || tk->type != TT_OP){
		s->invalid = 1;
		s->errStr = dup("Define not followed by op name");
		return tk;
	}
	
	return tk;
}
*/

Token *execAssociatedOp(State *s, Token *tk){
	Element *op = stackPoll(s->stack);
	if(op == NULL){
		return tk;
	}
	
	Token *exec = findMethod(op->methods, tk->s);
	if(exec == NULL){
		return tk;
	}
	
	Token *tkNext = tk->next;
	mql(s, exec);
	return tkNext;
}

Token *execGloablOp(State *s, Token *tk){
	Token *exec = findMethod(s->globalMethods, tk->s);
	if(exec == NULL){
		Element *sym = findSymbol(s->symbols, tk->s);
		if(sym != NULL){
			s->stack = stackPush(s->stack, dupElement(sym));
			return tk->next;
		}
		return tk;
	}
	
	Token *tkNext = tk->next;
	mql(s, exec);
	return tkNext;
}

Token* opMethods(State* s, Token* tk){
	printf("Builtin Ops	: ");
	printGlobalOps();
	printf("\n");
	
	printf("Global Ops: ");
	MethodList *tmp = NULL, *ent = NULL;
	HASH_ITER(hh, s->globalMethods, ent, tmp) {
		printf("%s \t", ent->name);
	}
	printf("\n");
	
	Element *op = stackPoll(s->stack);
	if(op != NULL){
		printf("Attached Ops: ");
		MethodList *tmp = NULL, *ent = NULL;
		HASH_ITER(hh, op->methods, ent, tmp) {
			printf("%s \t", ent->name);
		}
		printf("\n");
	}
	
	return tk;
}

Token* opBang(State* s, Token* tk){
	if(tk == NULL || tk->type != TT_OP){
		s->invalid = 1;
		s->errStr = dup("! Requires suceeding op token");
		return tk;
	}
	
	Element *op = stackPoll(s->stack);
	if(op != NULL){
		op = dupElement(op);
	}
	addSymbol(&s->symbols, tk->s, op);
	
	return tk->next;
}

void registerMethodOps(){
	registerGloablOp("ops", &opMethods);
	registerGloablOp("OPS", &opMethods);
	registerGloablOp("!", &opBang);
}

