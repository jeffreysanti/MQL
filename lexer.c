
#include "methods.h"

int isNumber(char c){
	return (c == '0' || c == '1' || c == '2' || c == '3' || 
			c == '4' || c == '5' || c == '6' || c == '7' || 
			c == '8' || c == '9');
}

int isWhitespace(char c){
	return (c == ' ' || c == '\n' || c == '\t');
}

int isSingleCharToken(char c){
	return (c == ',' || c == '[' || c == ']' || c == ':' || c == '{' || c == '}');
}

char *formStringToken(char *sstart, Token *tok){
	char endChar = sstart[0];
	char *s = sstart + 1;
	while(s[0] != 0 && (s[0] != endChar || s[-1] == '\\')){
		++s;
	}
	char *tokenstring = dupl(sstart+1, s);
	tok->s = tokenstring;
	tok->next = malloc(sizeof(Token));
	tok->next->type = TT_OP;
	tok->type = TT_STRING;
	
	return s;
}

char *formNumericToken(char *sstart, Token *tok){
	tok->type = TT_INT;
	int sawDecimal = (sstart[0] == '.');
	if(sawDecimal){
		tok->type = TT_FLOAT;
	}
	char *s = sstart + 1;
	while(!isWhitespace(s[0]) && s[0] != 0 && !isSingleCharToken(s[0])){
		if(s[0] == '.'){
			if(sawDecimal){
				tok->type = TT_OP; // error not number
			}else{
				tok->type = TT_FLOAT;
			}
		}
		else if(!isNumber(s[0])){
			tok->type = TT_OP; // error not number
		}
		++s;
	}
	char *tokenstring = dupl(sstart, s);
	if(!strcmp(tokenstring, ".") || !strcmp(tokenstring, "-")){
		tok->type = TT_OP; // pop stack operator
	}
	tok->s = tokenstring;
	tok->next = malloc(sizeof(Token));
	return s;
}

Token *tokenize(const char *s){
	char *remaining = (char*)s;
	char *token = (char*)s;
	Token *tok = malloc(sizeof(Token));
	tok->next = NULL;
	tok->s = NULL;
	tok->type = TT_OP;
	Token *start = tok;
	int comment = 0;
	while(1){
		if(remaining[0] == '#'){
			comment = 1;
		}
		if(remaining[0] == '\n' || remaining[0] == 0){
			comment = 0;
		}
		if(comment){
			++remaining;
			token = remaining;
			continue;
		}
		
		if(remaining[0] == '"' || remaining[0] == '\''){
			remaining = token = formStringToken(remaining, tok) + 1;
			tok = tok->next;
			tok->next = NULL;
			tok->s = NULL;
			tok->type = TT_OP;
		}
		if(remaining == token && (isNumber(remaining[0]) || remaining[0] == '.' || remaining[0] == '-')){
			remaining = token = formNumericToken(remaining, tok);
			tok = tok->next;
			tok->next = NULL;
			tok->s = NULL;
			tok->type = TT_OP;
		}
		
		//single character tokens
		if(isSingleCharToken(remaining[0])){
			if(remaining - token > 0){
				char *tokenstring = dupl(token, remaining);
				tok->s = tokenstring;
				tok->next = malloc(sizeof(Token));
				tok = tok->next;
				tok->next = NULL;
				tok->s = NULL;
				tok->type = TT_OP;
			}
			char *scToken = dupl(remaining, remaining+1);
			tok->s = scToken;
			
			tok->next = malloc(sizeof(Token));
			tok = tok->next;
			tok->next = NULL;
			tok->s = NULL;
			tok->type = TT_OP;
			
			token = remaining+1;
		}
		
		if(isWhitespace(remaining[0]) || remaining[0] == 0){
			if(remaining - token > 0){
				char *tokenstring = dupl(token, remaining);
				tok->s = tokenstring;
				tok->next = malloc(sizeof(Token));
				tok = tok->next;
				tok->next = NULL;
				tok->s = NULL;
				tok->type = TT_OP;
			}
			token = remaining+1;
		}
		if(remaining[0] == 0)
			break;
		++remaining;
	}
	
	tok = start;
	while(tok != NULL && tok->next != NULL){
		if(tok->next->s == NULL){
			free(tok->next);
			tok->next = NULL;
		}
		tok = tok->next;
	}
	if(start->s == NULL){
		free(start);
		return NULL;
	}
	
	State *state = newState();
	start = preprocessMethods(state, start);
	freeState(state);
	
	/*printf("LEX DUMP---\n");
	tok = start;
	while(tok != NULL){
		if(tok->type == TT_DEFINE){
			printf(" * DEFINE\n");
		}else if(tok->type == TT_CODEBLOCK){
			printf(" * CB\n");
		}else{
			printf(" > %s \n", tok->s);
		}
		tok = tok->next;
	}*/
	
	return start;
}

int isInputComplete(Token *tk){
	int defCount = 0;
	int endCount = 0;
	char *lastToken = "";
	
	while(tk != NULL){
		if(tk->type != TT_CODEBLOCK && tk->type != TT_DEFINE){
			if(!strcmp(tk->s, "{")){
				++defCount;
			}
			else if(!strcmp(tk->s, "}")){
				++endCount;
			}
			lastToken = tk->s;
		}
		
		tk = tk->next;
	}
	if(defCount != endCount || !strcmp(lastToken, "\\"))
		return 0;
	return 1;
}

void freeToken(Token *tk){
	if(tk->next != NULL)
		freeToken(tk->next);
	if(tk->type == TT_DEFINE && tk->s != NULL){
		TokenDefine *td = (TokenDefine *)tk->s;
		free(td->s);
		td->s = NULL;
	}
	if(tk->type == TT_CODEBLOCK && tk->s != NULL){
		tk->s = NULL; // it's just a number
	}
	if(tk->s != NULL)
		free(tk->s);
	free(tk);
}
