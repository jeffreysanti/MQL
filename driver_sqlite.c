
#include "methods.h"
#include "sqlite3.h"


Vector *dbList = NULL;

Token* opSqlite(State* s, Token* tk){
	Element *op1 = popStackOrErr(s);
	if(op1 == NULL){
		return tk;
	}
	
	if(op1->type != ET_STRING){
		s->invalid = 1;
		s->errStr = dup("sqlite must take string param");
		freeElement(op1);
		return tk;
	}
	
	sqlite3 *db;
	int ret = sqlite3_open((char*)op1->data, &db);
	if(ret != SQLITE_OK){
		s->invalid = 1;
		s->errStr = dup("Database open failed");
		freeElement(op1);
		return tk;
	}
	
	
	freeElement(op1);
	return tk;
}














void driver_sqlite(){
	registerGloablOp("SQLITE", &opSqlite);
	registerGloablOp("sqlite", &opSqlite);
}

void driverfree_sqlite(){
	
}

