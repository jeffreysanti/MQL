

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
	
	while(tk != NULL){
		if(s->invalid)
			return s;
		
		tk = mqlCodeBlock(s, tk);
	}
	return s;
}

Token *mqlCodeBlock(State *s, Token *tk){
	
	/*if(tk->type == TT_DEFINE){
		printf(" EXEC DEFINE\n");
	}else if(tk->type == TT_CODEBLOCK){
		printf(" EXEC CB (%d)\n", (int)tk->s);
	}else{
		printf(" EXEC %s \n", tk->s);
	}*/
	
	if(tk->type == TT_CODEBLOCK){
		Token *exec = codeBlockExecToken((int)tk->s);
		s = mql(s, exec);
		tk = tk->next;
	}else if(tk->type == TT_INT || tk->type == TT_FLOAT || tk->type == TT_STRING){
		tk = mqlProc_Elm(s, tk);
	}else if(tk->type == TT_DEFINE){
		tk = mqlProc_Def(s, tk);
	}else{
		tk = mql_op(s, tk);
	}
	return tk;
}

void init(){
	initMethodSpace();
	
	registerArithmeticOps();
	registerStackOps();
	registerElementOps();
	registerVectorOps();
	registerMethodOps();
	registerControlOps();
	
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
	
	
	/*char *line = ":fib dup dup 	0 = if . . 0 	else 1 = if . 1 	else dup 1 - fib swap 2 - fib + ; 20 fib";
	Token *tk = tokenize(line);
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
	*/
	
	freeState(state);
	cleanup();
}




