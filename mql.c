

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <stdlib.h>

#include "methods.h"


State *newState(){
	State *s = malloc(sizeof(State));
	s->invalid = 0;
	s->errStr = NULL;
	s->globalMethods = newMethodList();
	s->stack = newStack();
	s->vstack = newStack();
	s->symbols = newSymbolTable();
	s->tk = NULL;
	s->envs = NULL;
	
	return s;
}

void freeState(State *s){
	if(s == NULL)
		return;
	
	if(s->errStr != NULL){
		free(s->errStr);
	}
	freeMethodList(&s->globalMethods);
	freeSymbolTable(&s->symbols);
	s->stack = stackPopAll(s->stack);
	s->vstack = stackPopAll(s->vstack);
	
	free(s);
}

EnvStack *newEnv(EnvStack *s){
	EnvStack *s_new = malloc(sizeof(EnvStack));
	s_new->tk = NULL;
	s_new->parent = s;
	s_new->skipTokens = 0;
	return s_new;
}

EnvStack *envPop(EnvStack *s){
	if(s == NULL)
		return NULL;
	EnvStack *ret = s->parent;
	s->parent = NULL;
	free(s);
	return ret;
}

State *mql_s(State *s, const char *str){
	Token *tk = tokenize(str);
	s = mql(s, tk);
	freeToken(tk);
	return s;
}

void execGloablOp(State *s);
void execAssociatedOp(State *s);

void mql_op(State *s){
	Token *start = s->tk;
	execAssociatedOp(s);
	if(!s->invalid && s->tk == start){
		execGloablOp(s);
		if(!s->invalid && s->tk == start){
			execSuperGloablOp(s);
		}
	}
}

// mql executes until token list drained
State *mql(State *s, Token *tk){
	if(tk == NULL){
		return s;
	}

	if(s->tk != NULL){
		s->envs = newEnv(s->envs);
		s->envs->tk = s->tk->next;
	}
	s->tk = tk;

	while(s->tk != NULL){
		if(s->invalid)
			return s;
		
		mqlCodeBlock(s);
	}

	// restore old environment
	if(s->envs != NULL){
		s->tk = s->envs->tk;

		// skip tokens if needed
		for(int i=0; i<s->envs->skipTokens; ++i){
			if(s->tk == NULL){
				break;
			}
			s->tk = s->tk->next;
		}

		s->envs = envPop(s->envs);
	}

	return s;
}


// mqlCodeBlock limits execution to a single symbol or codeblock
void mqlCodeBlock(State *s){
	if(s->tk == NULL){
		return;
	}
	
	if(s->tk->type == TT_CODEBLOCK){
		Token *exec = codeBlockExecToken((int)s->tk->s);
		mql(s, exec);
	}else if(s->tk->type == TT_NUMBER || s->tk->type == TT_STRING){
		s->tk = mqlProc_Elm(s, s->tk);
	}else if(s->tk->type == TT_DEFINE){
		s->tk = mqlProc_Def(s, s->tk);
	}else{
		mql_op(s);
	}
}

void mqlCodeBlockNewEnv(State *s, Token *tk){
	s->tk = tk;
	mqlCodeBlock(s);
}

void init(){
	initMethodSpace();
	
	registerArithmeticOps();
	registerStackOps();
	registerElementOps();
	registerVectorOps();
	registerMethodOps();
	registerControlOps();
	registerBufferOps();
	registerManipOps();
	
	driver_sqlite();
}
void cleanup(){
	driverfree_sqlite();
	
	freeElementPool();
	freeGlobalOps();
	freeMethodSpace();
}

int main(int argc, char **argv){
	printf("MatrixQueryLanguage Version 0.1\n\n");
	init();
	int quit=0;
	
	State *state = newState();
	
	while(!quit){
		char *line = readline("MQL> ");
		if(!line){
			quit = 1;
			continue;
		}
		//line = trim(line);
		if(!strcmp(line, "quit") || !strcmp(line, "q")){
			quit = 1;
			continue;
		}
		
		
		Token *tk = tokenize(line);
		while(tk == NULL){ // incomplete input?
			char *line2 = readline(" > ");
			line = realloc(line, strlen(line) + strlen(line2) + 1 + 1);
			strcat(line, "\n");
			strcat(line, line2);
			tk = tokenize(line);
			free(line2);
		}
		add_history(line);
		
		state = mql(state, tk);
		if(state->invalid){
			printf("Error: %s \n", state->errStr);
			state->invalid = 0;
			free(state->errStr);
			state->errStr = NULL;
		}
		
		Element *top = stackPoll(state->stack);
		printElement(top);
		
		if(tk != NULL)
			freeToken(tk);
		free(line);
	}
	
	freeState(state);
	cleanup();
}




