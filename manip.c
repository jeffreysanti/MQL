#include "methods.h"



// Methods designed for manipulating tokens at run time
// Suppose we want to implment filter in mql
//               filter {2 >}
//    ====>      dup 2 > if {} else .
// so ====>      :filter2 { dup 1 %after if {} else . 1 %skip}
// 1 #after executes the first code block after the filter token
// 1 #skip, skips past the code block which is executed (sets a skip register in the state)



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

}



