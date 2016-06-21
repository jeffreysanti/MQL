

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

State *mql_s(State *s, const char *str){
	Token *tk = tokenize(str);
	s = mql(s, tk);
	freeToken(tk);
}

Token *execGloablOp(State *s, Token *tk);
Token *execAssociatedOp(State *s, Token *tk);

Token *mql_op(State *s, Token *tk){
	Token *start = tk;
	tk = execAssociatedOp(s, tk);
	if(!s->invalid && tk == start){
		tk = execGloablOp(s, tk);
		if(!s->invalid && tk == start){
			tk = execSuperGloablOp(s, tk);
		}
	}
	return tk;
}

State *mql(State *s, Token *tk){
	if(s == NULL){
		s = newState();
	}
	
	tk = preprocessMethods(s, tk);
	
	while(tk != NULL){
		if(s->invalid)
			return s;
		
		if(tk->type == TT_INT || tk->type == TT_FLOAT || tk->type == TT_STRING){
			tk = mqlProc_Elm(s, tk);
		}else if(tk->type == TT_DEFINE){
			tk = mqlProc_Def(s, tk);
		}else{
			tk = mql_op(s, tk);
		}
	
	}
	return s;
}

void init(){
	initMethodSpace();
	
	registerArithmeticOps();
	registerStackOps();
	registerElementOps();
	registerMethodOps();
	registerControlOps();
}
void cleanup(){
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
		while(!isInputComplete(tk)){
			freeToken(tk);
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




