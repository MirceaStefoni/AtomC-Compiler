#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"
#include "ad.h"

Domain *symTable=NULL;
FILE *ad_log_fp = NULL; // Define the global file pointer

// Function to initialize the domain analysis log file
void initAdLog() {
	ad_log_fp = fopen("results/ad.txt", "w");
	if (ad_log_fp == NULL) {
		perror("Error opening results/ad.txt");
		// Continue without file logging, output will go to stdout via checks
	}
}

// Function to close the domain analysis log file
void closeAdLog() {
	if (ad_log_fp != NULL) {
		fclose(ad_log_fp);
		ad_log_fp = NULL;
	}
}

int typeBaseSize(Type *t){
	switch(t->tb){
		case TB_INT:return sizeof(int);
		case TB_DOUBLE:return sizeof(double);
		case TB_CHAR:return sizeof(char);
		case TB_VOID:return 0;
		default:{		// TB_STRUCT
			int size=0;
			for(Symbol *m=t->s->structMembers;m;m=m->next){
				size+=typeSize(&m->type);
				}
			return size;
			}
		}
	}

int typeSize(Type *t){
	if(t->n<0)return typeBaseSize(t);
	if(t->n==0)return sizeof(void*);
	return t->n*typeBaseSize(t);
	}

// free from memory a list of symbols
void freeSymbols(Symbol *list){
	for(Symbol *next;list;list=next){
		next=list->next;
		freeSymbol(list);
		}
	}

Symbol *newSymbol(const char *name,SymKind kind){
	Symbol *s=(Symbol*)safeAlloc(sizeof(Symbol));
	// sets all the fields to 0/NULL
	memset(s,0,sizeof(Symbol));
	s->name=name;
	s->kind=kind;
	return s;
	}

Symbol *dupSymbol(Symbol *symbol){
	Symbol *s=(Symbol*)safeAlloc(sizeof(Symbol));
	*s=*symbol;
	s->next=NULL;
	return s;
	}

// s->next is already NULL from newSymbol
Symbol *addSymbolToList(Symbol **list,Symbol *s){
	Symbol *iter=*list;
	if(iter){
		while(iter->next)iter=iter->next;
		iter->next=s;
		}else{
		*list=s;
		}
	return s;
	}

int symbolsLen(Symbol *list){
	int n=0;
	for(;list;list=list->next)n++;
	return n;
	}

void freeSymbol(Symbol *s){
	switch(s->kind){
		case SK_VAR:
			if(!s->owner)free(s->varMem);
			break;
		case SK_PARAM:
			// DO nothing
			break;
		case SK_FN:
			freeSymbols(s->fn.params);
			freeSymbols(s->fn.locals);
			break;
		case SK_STRUCT:
			freeSymbols(s->structMembers);
			break;
		}
	free(s);
	}

Domain *pushDomain(){
	Domain *d=(Domain*)safeAlloc(sizeof(Domain));
	d->symbols=NULL;
	d->parent=symTable;
	symTable=d;
	return d;
	}

void dropDomain(){
	Domain *d=symTable;
	symTable=d->parent;
	freeSymbols(d->symbols);
	free(d);
	}

void showNamedType(Type *t,const char *name){
	FILE* out = ad_log_fp ? ad_log_fp : stdout; // Determine output stream
	switch(t->tb){
		case TB_INT:fprintf(out, "int");break;
		case TB_DOUBLE:fprintf(out, "double");break;
		case TB_CHAR:fprintf(out, "char");break;
		case TB_VOID:fprintf(out, "void");break;
		default:		// TB_STRUCT
			fprintf(out, "struct %s",t->s->name);
		}
	if(name)fprintf(out, " %s",name);
	if(t->n==0)fprintf(out, "[]");
	else if(t->n>0)fprintf(out, "[%d]",t->n);
	}

void showSymbol(Symbol *s){
	FILE* out = ad_log_fp ? ad_log_fp : stdout; // Determine output stream
	switch(s->kind){
			case SK_VAR:
				showNamedType(&s->type,s->name);
				if(s->owner){
					fprintf(out, ";\t// size=%d, idx=%d\n",typeSize(&s->type),s->varIdx);
					}else{
					fprintf(out, ";\t// size=%d, mem=%p\n",typeSize(&s->type),s->varMem);
					}
				break;
			case SK_PARAM:{
				showNamedType(&s->type,s->name);
				fprintf(out, " /*size=%d, idx=%d*/",typeSize(&s->type),s->paramIdx);
				}break;
			case SK_FN:{
				showNamedType(&s->type,s->name);
				fprintf(out, "(");
				bool next=false;
				for(Symbol *param=s->fn.params;param;param=param->next){
					if(next)fprintf(out, ", ");
					showSymbol(param); // Recursive call will use 'out'
					next=true;
					}
				fprintf(out, "){\n");
				for(Symbol *local=s->fn.locals;local;local=local->next){
					fprintf(out, "\t");
					showSymbol(local); // Recursive call will use 'out'
					}
				fprintf(out, "\t}\n");
				}break;
			case SK_STRUCT:{
				fprintf(out, "struct %s{\n",s->name);
				for(Symbol *m=s->structMembers;m;m=m->next){
					fprintf(out, "\t");
					showSymbol(m); // Recursive call will use 'out'
					}
				fprintf(out, "\t};\t// size=%d\n",typeSize(&s->type));
				}break;
		}
	}

void showDomain(Domain *d,const char *name){
	FILE* out = ad_log_fp ? ad_log_fp : stdout; // Determine output stream
	fprintf(out, "// domain: %s\n",name);
	for(Symbol *s=d->symbols;s;s=s->next){
		showSymbol(s); // This will use 'out' correctly
		}
	fprintf(out, "\n");
	}

Symbol *findSymbolInDomain(Domain *d,const char *name){
	for(Symbol *s=d->symbols;s;s=s->next){
		if(!strcmp(s->name,name))return s;
		}
	return NULL;
	}

Symbol *findSymbol(const char *name){
	for(Domain *d=symTable;d;d=d->parent){
		Symbol *s=findSymbolInDomain(d,name);
		if(s)return s;
		}
	return NULL;
	}

Symbol *addSymbolToDomain(Domain *d,Symbol *s){
	return addSymbolToList(&d->symbols,s);
	}

Symbol *addExtFn(const char *name,void(*extFnPtr)(),Type ret){
	Symbol *fn=newSymbol(name,SK_FN);
	fn->fn.extFnPtr=extFnPtr;
	fn->type=ret;
	addSymbolToDomain(symTable,fn);
	return fn;
	}

Symbol *addFnParam(Symbol *fn,const char *name,Type type){
	Symbol *param=newSymbol(name,SK_PARAM);
	param->type=type;
	param->paramIdx=symbolsLen(fn->fn.params);
	addSymbolToList(&fn->fn.params,dupSymbol(param));
	return param;
	}
