
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

Token *execSuperGloablOp(State *s, Token *tk){
	GlobalOpList *x;
	char *str = tk->s;
	
	HASH_FIND_STR(_GOL, str, x);
	if(x == NULL){
		// now assume it's a string
		tk->type = TT_STRING;
		tk = mqlProc_Elm(s, tk);
		return tk;
	}
	
	tk = tk->next;
	tk = x->func(s, tk);
	return tk;
}

void printGlobalOps(){
	GlobalOpList *tmp = NULL, *ent = NULL;
	HASH_ITER(hh, _GOL, ent, tmp) {
		printf("%s \t", ent->name);
	}
}


