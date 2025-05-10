#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>
#include "ad.h"
#include "at.h"

extern Token *iTk;    
extern Token *consumedTk; 

void initParserLog(void);
void closeParserLog(void);

void tkerr(const char *fmt, ...);
bool consume(int code);

bool unit();
bool structDef();
bool varDef();
bool typeBase();
bool arrayDecl();
bool fnDef();
bool fnParam();
bool stm();
bool stmCompound(bool newDomain); // newDomain parameter is for syntactic scope, owner handles semantic
bool expr(Ret *r);
bool exprAssign(Ret *r);
bool exprOr(Ret *r);
bool exprOrPrim(Ret *r);
bool exprAnd(Ret *r);
bool exprAndPrim(Ret *r); 
bool exprEq(Ret *r);
bool exprEqPrim(Ret *r); 
bool exprRel(Ret *r);
bool exprRelPrim(Ret *r);
bool exprAdd(Ret *r);
bool exprAddPrim(Ret *r); 
bool exprMul(Ret *r);
bool exprMulPrim(Ret *r); 
bool exprCast(Ret *r);
bool exprUnary(Ret *r);
bool exprPostfix(Ret *r);
bool exprPostfixPrim(Ret *r); 
bool exprPrimary(Ret *r);


void parse(Token *tokens);

#endif