#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "parser.h"

Token *iTk;		// the iterator in the tokens list
Token *consumedTk;		// the last consumed token

void tkerr(const char *fmt,...){
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

bool consume(int code){
	if(iTk->code==code){
		consumedTk=iTk;
		iTk=iTk->next;
		return true;
		}
	return false;
	}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(){
	if(consume(TYPE_INT)){
		return true;
		}
	if(consume(TYPE_DOUBLE)){
		return true;
		}
	if(consume(TYPE_CHAR)){
		return true;
		}
	if(consume(STRUCT)){
		if(consume(ID)){
			return true;
			}
		}
	return false;
	}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl() {
    Token *start = iTk;
    if (consume(LBRACKET)) {
        consume(INT); // optional
        if (consume(RBRACKET)) return true;
        tkerr("missing ] after array declaration");
    }
    iTk = start;
    return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef() {
    Token *start = iTk;
    if (typeBase()) {
        if (consume(ID)) {
            arrayDecl(); // optional
            if (consume(SEMICOLON)) return true;
            tkerr("missing ; after variable definition");
        } else {
            tkerr("missing identifier in variable definition");
        }
    }
    iTk = start;
    return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef() {
    Token *start = iTk;  // Salvăm poziția inițială a iteratorului
    if (consume(STRUCT)) {
        if (consume(ID)) {
            if (iTk->code == LACC) {  
                consume(LACC);  
                while (varDef()) {} 
                if (consume(RACC)) {
                    if (consume(SEMICOLON)) return true;
                    tkerr("missing ; after struct definition");
                } else {
                    tkerr("missing } in struct definition");
                }
            } else {
                iTk = start; 
                return false;
            }
        } else {
            tkerr("missing identifier after struct");
        }
    }
    iTk = start;  
    return false;
}

// fnParam: typeBase ID arrayDecl?
bool fnParam() {
    Token *start = iTk;
    if (typeBase()) {
        if (consume(ID)) {
            arrayDecl(); // optional
            return true;
        } else {
            tkerr("missing identifier in function parameter");
        }
    }
    iTk = start;
    return false;
}

// fnDef: (typeBase | VOID) ID LPAR (fnParam (COMMA fnParam)*)? RPAR stmCompound
bool fnDef() {
    Token *start = iTk;  
    if (typeBase() || consume(VOID)) { 
        if (consume(ID)) {  
            if (iTk->code == LPAR) {  
                consume(LPAR);  
                if (fnParam()) {
                    while (consume(COMMA)) {
                        if (!fnParam()) {
                            tkerr("missing function parameter after comma");
                        }
                    }
                }
                if (consume(RPAR)) {
                    if (stmCompound()) return true;
                    tkerr("missing compound statement in function definition");
                } else {
                    tkerr("missing ) after function parameters");
                }
            } else {
                iTk = start;  
                return false;
            }
        } else {
            tkerr("missing identifier in function definition");
        }
    }
    iTk = start;  
    return false;
}

// stmCompound: LACC (varDef | stm)* RACC
bool stmCompound() {
    Token *start = iTk;
    if (consume(LACC)) {
        for (;;) {
            if (varDef()) {}
            else if (stm()) {}
            else break;
        }
        if (consume(RACC)) return true;
        tkerr("missing } in compound statement");
    }
    iTk = start;
    return false;
}

// stm: stmCompound | IF LPAR expr RPAR stm (ELSE stm)? | WHILE LPAR expr RPAR stm | RETURN expr? SEMICOLON | expr? SEMICOLON
bool stm() {
    Token *start = iTk;
    if (stmCompound()) return true;
    
    if (consume(IF)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) {
                        if (consume(ELSE)) {
                            if (stm()) return true;
                            tkerr("missing statement after else");
                        }
                        return true; // else is optional
                    } else {
                        tkerr("missing statement after if");
                    }
                } else {
                    tkerr("missing ) after if condition");
                }
            } else {
                tkerr("missing expression in if condition");
            }
        } else {
            tkerr("missing ( after if");
        }
    }
    
    if (consume(WHILE)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) return true;
                    tkerr("missing statement after while");
                } else {
                    tkerr("missing ) after while condition");
                }
            } else {
                tkerr("missing expression in while condition");
            }
        } else {
            tkerr("missing ( after while");
        }
    }
    
    if (consume(RETURN)) {
        expr(); // optional
        if (consume(SEMICOLON)) return true;
        tkerr("missing ; after return");
    }
    
    expr(); // optional
    if (consume(SEMICOLON)) return true;
    
    iTk = start;
    return false;
}

// expr: exprAssign
bool expr() {
    return exprAssign();
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign() {
    Token *start = iTk;
    if (exprUnary()) {
        if (consume(ASSIGN)) {
            if (exprAssign()) return true;
            tkerr("missing expression after =");
        } else {
            iTk = start;
            if (exprOr()) return true;
        }
    } else {
        if (exprOr()) return true;
    }
    return false;
}

// exprOr: exprAnd exprOrPrim (eliminated left recursion from exprOr: exprOr OR exprAnd | exprAnd)
bool exprOr() {
    if (exprAnd()) {
        if (exprOrPrim()) return true;
    }
    return false;
}

bool exprOrPrim() {
    if (consume(OR)) {
        if (exprAnd()) {
            if (exprOrPrim()) return true;
            tkerr("missing expression after ||");
        } else {
            tkerr("missing expression after ||");
        }
    }
    return true; // epsilon
}

// exprAnd: exprEq exprAndPrim (eliminated left recursion from exprAnd: exprAnd AND exprEq | exprEq)
bool exprAnd() {
    if (exprEq()) {
        if (exprAndPrim()) return true;
    }
    return false;
}

bool exprAndPrim() {
    if (consume(AND)) {
        if (exprEq()) {
            if (exprAndPrim()) return true;
            tkerr("missing expression after &&");
        } else {
            tkerr("missing expression after &&");
        }
    }
    return true; // epsilon
}

// exprEq: exprRel exprEqPrim (eliminated left recursion from exprEq: exprEq (EQUAL | NOTEQ) exprRel | exprRel)
bool exprEq() {
    if (exprRel()) {
        if (exprEqPrim()) return true;
    }
    return false;
}

bool exprEqPrim() {
    if (consume(EQUAL) || consume(NOTEQ)) {
        if (exprRel()) {
            if (exprEqPrim()) return true;
            tkerr("missing expression after equality operator");
        } else {
            tkerr("missing expression after equality operator");
        }
    }
    return true; // epsilon
}

// exprRel: exprAdd exprRelPrim (eliminated left recursion from exprRel: exprRel (LESS | LESSEQ | GREATER | GREATEREQ) exprAdd | exprAdd)
bool exprRel() {
    if (exprAdd()) {
        if (exprRelPrim()) return true;
    }
    return false;
}

bool exprRelPrim() {
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
        if (exprAdd()) {
            if (exprRelPrim()) return true;
            tkerr("missing expression after relational operator");
        } else {
            tkerr("missing expression after relational operator");
        }
    }
    return true; // epsilon
}

// exprAdd: exprMul exprAddPrim (eliminated left recursion from exprAdd: exprAdd (ADD | SUB) exprMul | exprMul)
bool exprAdd() {
    if (exprMul()) {
        if (exprAddPrim()) return true;
    }
    return false;
}

bool exprAddPrim() {
    if (consume(ADD) || consume(SUB)) {
        if (exprMul()) {
            if (exprAddPrim()) return true;
            tkerr("missing expression after additive operator");
        } else {
            tkerr("missing expression after additive operator");
        }
    }
    return true; // epsilon
}

// exprMul: exprCast exprMulPrim (eliminated left recursion from exprMul: exprMul (MUL | DIV) exprCast | exprCast)
bool exprMul() {
    if (exprCast()) {
        if (exprMulPrim()) return true;
    }
    return false;
}

bool exprMulPrim() {
    if (consume(MUL) || consume(DIV)) {
        if (exprCast()) {
            if (exprMulPrim()) return true;
            tkerr("missing expression after multiplicative operator");
        } else {
            tkerr("missing expression after multiplicative operator");
        }
    }
    return true; // epsilon
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast() {
    Token *start = iTk;
    if (consume(LPAR)) {
        if (typeBase()) {
            arrayDecl(); // optional
            if (consume(RPAR)) {
                if (exprCast()) return true;
                tkerr("missing expression after cast");
            } else {
                tkerr("missing ) after cast type");
            }
        }
        iTk = start;
    }
    return exprUnary();
}

// exprUnary: (SUB | NOT) exprUnary | exprPostfix
bool exprUnary() {
    if (consume(SUB) || consume(NOT)) {
        if (exprUnary()) return true;
        tkerr("missing expression after unary operator");
    }
    return exprPostfix();
}

// exprPostfix: exprPrimary exprPostfixPrim (eliminated left recursion from exprPostfix: exprPostfix LBRACKET expr RBRACKET | exprPostfix DOT ID | exprPrimary)
bool exprPostfix() {
    if (exprPrimary()) {
        if (exprPostfixPrim()) return true;
    }
    return false;
}

bool exprPostfixPrim() {
    if (consume(LBRACKET)) {
        if (expr()) {
            if (consume(RBRACKET)) {
                if (exprPostfixPrim()) return true;
            } else {
                tkerr("missing ] after array index");
            }
        } else {
            tkerr("missing expression in array index");
        }
    } else if (consume(DOT)) {
        if (consume(ID)) {
            if (exprPostfixPrim()) return true;
        } else {
            tkerr("missing identifier after .");
        }
    }
    return true; // epsilon
}

// exprPrimary: ID (LPAR (expr (COMMA expr)*)? RPAR)? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary() {
    // Token *start = iTk;
    if (consume(ID)) {
        if (consume(LPAR)) {
            if (expr()) {
                while (consume(COMMA)) {
                    if (!expr()) {
                        tkerr("missing expression after ,");
                    }
                }
            }
            if (consume(RPAR)) return true;
            tkerr("missing ) after function arguments");
        }
        return true; // just ID
    }
    if (consume(INT) || consume(DOUBLE) || consume(CHAR) || consume(STRING)) return true;
    if (consume(LPAR)) {
        if (expr()) {
            if (consume(RPAR)) return true;
            tkerr("missing ) after expression");
        } else {
            tkerr("missing expression after (");
        }
    }
    return false;
}

// unit: (structDef | fnDef | varDef)* END
bool unit() {
    for (;;) {
        if (structDef()) {}
        else if (fnDef()) {}
        else if (varDef()) {}
        else break;
    }
    if (consume(END)) return true;
    tkerr("missing END at the end of the program");
    return false;
}

void parse(Token *tokens) {
    iTk = tokens;
    if (!unit()) tkerr("syntax error");
}