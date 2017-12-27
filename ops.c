
#include "methods.h"


typedef struct GlobalOpList GlobalOpList;
struct GlobalOpList {
	char *name;
	Token* (*func)(State*, Token*);
	UT_hash_handle hh;
};

GlobalOpList *_GOL = NULL;

void registerGloablOp(char *str, Token* (*func)(State*, Token*)){
	GlobalOpList *x;
	
	HASH_FIND_STR(_GOL, str, x);
	if(x != NULL){
		printf("Cannnot register global op '%s' twice!\n", str);
		exit(1);
	}
	
	x = malloc(sizeof(GlobalOpList));
	x->name = dup(str);
	x->func = func;
	HASH_ADD_KEYPTR(hh, _GOL, x->name, strlen(x->name), x);
	
}

void freeGlobalOps(){
	GlobalOpList *tmp, *ent;
	HASH_ITER(hh, _GOL, ent, tmp) {
		HASH_DEL(_GOL, ent);
		free(ent->name);
		free(ent);
	}
}

void execSuperGloablOp(State *s){
	GlobalOpList *x;
	char *str = s->tk->s;
	
	HASH_FIND_STR(_GOL, str, x);
	if(x == NULL){
		// now assume it's a string
		s->tk->type = TT_STRING;
		s->tk = mqlProc_Elm(s, s->tk);
		return;
	}
	
	s->tk = s->tk->next;
	s->tk = x->func(s, s->tk);
}

void printGlobalOps(){
	GlobalOpList *tmp = NULL, *ent = NULL;
	HASH_ITER(hh, _GOL, ent, tmp) {
		printf("%s \t", ent->name);
	}
}


