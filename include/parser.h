#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>

extern Token *iTk;    
extern Token *consumedTk; 

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
bool stmCompound();
bool expr();
bool exprAssign();
bool exprOr();
bool exprOrPrim();
bool exprAnd();
bool exprAndPrim();
bool exprEq();
bool exprEqPrim();
bool exprRel();
bool exprRelPrim();
bool exprAdd();
bool exprAddPrim();
bool exprMul();
bool exprMulPrim();
bool exprCast();
bool exprUnary();
bool exprPostfix();
bool exprPostfixPrim();
bool exprPrimary();


void parse(Token *tokens);

#endif