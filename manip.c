#include "methods.h"



// Methods designed for manipulating tokens at run time
// Suppose we want to implment filter in mql
//               filter {2 >}
//    ====>      dup 2 > if {} else .
// so ====>      :filter2 { dup 1 %after if {} else . 1 %skip}
// 1 #after executes the first code block after the filter token
// 1 #skip, skips past the code block which is executed (sets a skip register in the state)


// Insert: Modifies the following codeblocks

Token * manipAfter(State* s, Token* tk, int envsBack){
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op1->type != ET_NUMBER){
		s->invalid = 1;
		s->errStr = dup("%after requires a numeric index on the stack");
		return tk;
	}
	int idx = op1->dval;

	if(idx < 1){
		return tk; // nothing to do
	}

	// go back n environments
	EnvStack *env = s->envs;
	for(int i=1; i<envsBack; ++i){
		if(env == NULL){
			s->invalid = 1;
			s->errStr = dup("%after failed because the environment does not exist");
			freeElement(op1);
			return tk;
		}
		env = env->parent;
	}
	if(env == NULL){
		s->invalid = 1;
		s->errStr = dup("%after failed because the environment does not exist");
		freeElement(op1);
		return tk;
	}

	Token *doExec = env->tk;
	for(int i=1; i<idx; ++i){
		if(doExec == NULL){
			s->invalid = 1;
			s->errStr = dup("%after failed because required number of tokens not available");
			freeElement(op1);
			return tk;
		}
		doExec = doExec->next;
	}
	if(doExec == NULL){
		s->invalid = 1;
		s->errStr = dup("%after failed because required number of tokens not available");
		freeElement(op1);
		return tk;
	}

	// execute the code block (in it's own environment)
	mqlCodeBlockNewEnv(s, doExec);
	
	freeElement(op1);
	return tk;
}

Token * manipSkip(State* s, Token* tk, int envsBack){
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL || op1->type != ET_NUMBER){
		s->invalid = 1;
		s->errStr = dup("%skip requires a numeric count on the stack");
		return tk;
	}
	int cnt = op1->dval;

	// go back n environments
	EnvStack *env = s->envs;
	for(int i=1; i<envsBack; ++i){
		if(env == NULL){
			s->invalid = 1;
			s->errStr = dup("%skip failed because the environment does not exist");
			freeElement(op1);
			return tk;
		}
		env = env->parent;
	}
	if(env == NULL){
		s->invalid = 1;
		s->errStr = dup("%skip failed because the environment does not exist");
		freeElement(op1);
		return tk;
	}

	env->skipTokens = cnt;
	
	freeElement(op1);
	return tk;
}


Token * opManipAfter1(State* s, Token* tk){
	return manipAfter(s, tk, 1);
}
Token * opManipSkip1(State* s, Token* tk){
	return manipSkip(s, tk, 1);
}
Token * opManipAfter2(State* s, Token* tk){
	return manipAfter(s, tk, 2);
}
Token * opManipSkip2(State* s, Token* tk){
	return manipSkip(s, tk, 2);
}
Token * opManipAfter3(State* s, Token* tk){
	return manipAfter(s, tk, 3);
}
Token * opManipSkip3(State* s, Token* tk){
	return manipSkip(s, tk, 3);
}



Token *manipInsert(State *s, Token *tk, int order, char exec){
	// order refers to the nesting of code blocks
	// order 0 is at this scope (has 1 param where 0=insert immediately after, 1=skip one word/cb)
	// order 1 is the first level of code blocks (has 2 code params: 
	//		first is code block index (0,1,2,...), second is word index (0 for first word, 1 is skip one etc)
	// Examples: (^ is where word code inserted)
	// 3 Order0 : 3 insert {...} A B C ^ D E F
	// 1,3 Order2 : 3 insert {...} A {B C  D } E F G H I { J K L ^ M N O } P Q R
	// Execute controls whether the {...} is executed first, and resulting stack is inserted or if tokens directly inserted

	// Note: Inserts are destructive: They will only execute once and remove themselves from the codeblock by
	// subsituting a nop for the insertion after executing

	int idxs[8] = {0,0,0,0,0,0,0,0};
	for(int i=order; i>=0; --i){
		Element *op1 = popStackOrErr(s);
		if(op1 == NULL || op1->type != ET_NUMBER){
			s->invalid = 1;
			s->errStr = dup("%insert requires numeric indicies on the stack");
			printf("Insert Failed: i= %d\n", i);
			return tk;
		}
		idxs[i] = (int)op1->dval;
		freeElement(op1);
	}

	if(tk == NULL){
		s->invalid = 1;
		s->errStr = dup("%insert failed because a code block for insertion did not follow it");
		printf("TK IS NULL\n");
		return tk;
	}

	printf("Insert Okay: Order %d\n", order);

	Token *tkCont = tk->next;

	// If the following token is a nop, this is inert
	if(tk->type == TT_OP && !strcmp(tk->s, "nop")){
		return tkCont;
	}
	
	// figure out what to insert
	Token *insertionChain = NULL;
	if(exec){
		Stack *oldStack = s->stack;
		s->stack = newStack();
		mqlCodeBlockNewEnv(s, tk);
		insertionChain = newStackDataToken(s->stack);
		s->stack = oldStack;
	}else{
		if(tk->type == TT_CODEBLOCK){
			insertionChain = unpackCodeBlock((int)tk->s);
		}else{
			insertionChain = duplicateToken(tk);
		}
	}

	// perform insertion
	// first find the environment,
	// then find the placement
	Token *tkPointer = tkCont;
	for(int i=0; i<order; ++i){
		// locate codeblock
		int blockNo = idxs[i];
		int blksFound=0;
		while(tkPointer != NULL){
			if(tkPointer->type == TT_CODEBLOCK){
				if(blksFound == blockNo){
					blksFound = -1;
					break;
				}
				blksFound++;
			}
			tkPointer = tkPointer->next;
		}
		if(blksFound != -1){
			s->invalid = 1;
			s->errStr = dup("%insert failed to find codeblock to insert into");
			return tkCont;
		}
		
		// enter found codeblock
		tkPointer = codeBlockExecToken((int)tkPointer->s);
	}
	
	// get to the insert point
	for(int i=0; i<idxs[order]; ++i){
		if(tkPointer == NULL){
			s->invalid = 1;
			s->errStr = dup("%insert failed to find insertion point within codeblock. Ran out of tokens.");
			return tkCont;
		}
		tkPointer = tkPointer->next;
	}
	if(tkPointer == NULL){
		s->invalid = 1;
		s->errStr = dup("%insert failed to find insertion point within codeblock. Ran out of tokens.");
		return tkCont;
	}

	// and insert
	Token *next = tkPointer->next;
	tkPointer->next = insertionChain;
	while(insertionChain->next != NULL){
		insertionChain = insertionChain->next;
	}
	insertionChain->next = next;

	// make it inert now, so we never insert again
	freeTokenData(tk);
	tk->s = dup("nop");
	tk->type = TT_OP;
	return tkCont;
}

Token * opManipInsert0(State* s, Token* tk){
	return manipInsert(s, tk, 0, 0);
}
Token * opManipInsert0X(State* s, Token* tk){
	return manipInsert(s, tk, 0, 1);
}
Token * opManipInsert1(State* s, Token* tk){
	return manipInsert(s, tk, 1, 0);
}
Token * opManipInsert1X(State* s, Token* tk){
	return manipInsert(s, tk, 1, 1);
}
Token * opManipInsert2(State* s, Token* tk){
	return manipInsert(s, tk, 2, 0);
}
Token * opManipInsert2X(State* s, Token* tk){
	return manipInsert(s, tk, 2, 1);
}
Token * opManipInsert3(State* s, Token* tk){
	return manipInsert(s, tk, 3, 0);
}
Token * opManipInsert3X(State* s, Token* tk){
	return manipInsert(s, tk, 3, 1);
}



void registerManipOps(){
	registerGloablOp("%after", &opManipAfter1);
	registerGloablOp("%AFTER", &opManipAfter1);
	registerGloablOp("%afterr", &opManipAfter2);
	registerGloablOp("%AFTERR", &opManipAfter2);
	registerGloablOp("%afterrr", &opManipAfter3);
	registerGloablOp("%AFTERRR", &opManipAfter3);

	registerGloablOp("%skip", &opManipSkip1);
	registerGloablOp("%SKIP", &opManipSkip1);
	registerGloablOp("%skipp", &opManipSkip2);
	registerGloablOp("%SKIPP", &opManipSkip2);
	registerGloablOp("%skippp", &opManipSkip3);
	registerGloablOp("%SKIPPP", &opManipSkip3);

	registerGloablOp("%insert", &opManipInsert0);
	registerGloablOp("%INSERT", &opManipInsert0);
	registerGloablOp("%insertx", &opManipInsert0X);
	registerGloablOp("%INSERTX", &opManipInsert0X);
	registerGloablOp("%insertt", &opManipInsert1);
	registerGloablOp("%INSERTT", &opManipInsert1);
	registerGloablOp("%inserttx", &opManipInsert1X);
	registerGloablOp("%INSERTTX", &opManipInsert1X);
	registerGloablOp("%inserttt", &opManipInsert2);
	registerGloablOp("%INSERTTT", &opManipInsert2);
	registerGloablOp("%insertttx", &opManipInsert2X);
	registerGloablOp("%INSERTTTX", &opManipInsert2X);
	registerGloablOp("%insertttt", &opManipInsert3);
	registerGloablOp("%INSERTTTT", &opManipInsert3);
	registerGloablOp("%inserttttx", &opManipInsert3X);
	registerGloablOp("%INSERTTTTX", &opManipInsert3X);
}



