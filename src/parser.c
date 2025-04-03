#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h> 

#include "parser.h"
#include "lexer.h" 

// Global file pointer for parser log
FILE *parser_log_fp = NULL;

// Function to initialize the parser log file
void initParserLog() {
    parser_log_fp = fopen("results/parser.txt", "w");
    if (parser_log_fp == NULL) {
        perror("Error opening results/parser.txt");
        exit(EXIT_FAILURE); // Exit if we can't open the log file
    }
}

// Function to close the parser log file
void closeParserLog() {
    if (parser_log_fp != NULL) {
        fclose(parser_log_fp);
        parser_log_fp = NULL;
    }
}

const char *tkCodeName(int code) {
    switch (code) {
        case ID: return "ID";
        case TYPE_INT: return "TYPE_INT";
        case TYPE_DOUBLE: return "TYPE_DOUBLE";
        case TYPE_CHAR: return "TYPE_CHAR";
        case STRUCT: return "STRUCT";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case WHILE: return "WHILE";
        case RETURN: return "RETURN";
        case VOID: return "VOID";
        case INT: return "INT";
        case DOUBLE: return "DOUBLE";
        case CHAR: return "CHAR";
        case STRING: return "STRING";
        case COMMA: return "COMMA";
        case SEMICOLON: return "SEMICOLON";
        case LPAR: return "LPAR";
        case RPAR: return "RPAR";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case LACC: return "LACC";
        case RACC: return "RACC";
        case END: return "END";
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case DOT: return "DOT";
        case ASSIGN: return "ASSIGN";
        case EQUAL: return "EQUAL";
        case NOTEQ: return "NOTEQ";
        case LESS: return "LESS";
        case LESSEQ: return "LESSEQ";
        case GREATER: return "GREATER";
        case GREATEREQ: return "GREATEREQ";
        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";
        default: return "UNKNOWN";
    }
}

void rule_start(const char *ruleName) {
    if (parser_log_fp) fprintf(parser_log_fp, "Enter %s\n", ruleName);
}


void rule_end(const char *ruleName, bool success) {
   
    if (parser_log_fp) fprintf(parser_log_fp, "Exit %s (%s)\n", ruleName, success ? "OK" : "FAIL");
}


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
    if (parser_log_fp) fprintf(parser_log_fp, "consume(%s)", tkCodeName(code));
    if(iTk->code==code){
        consumedTk=iTk;
        iTk=iTk->next;
        if (parser_log_fp) fprintf(parser_log_fp, " => consumed\n");
        return true;
    }
    if (parser_log_fp) fprintf(parser_log_fp, " => found %s\n", tkCodeName(iTk->code));
    return false;
}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(){
    rule_start("typeBase");
    Token *start = iTk;
	if(consume(TYPE_INT)){
        rule_end("typeBase", true);
		return true;
		}
	if(consume(TYPE_DOUBLE)){
        rule_end("typeBase", true);
		return true;
		}
	if(consume(TYPE_CHAR)){
        rule_end("typeBase", true);
		return true;
		}
	if(consume(STRUCT)){
		if(consume(ID)){
            rule_end("typeBase", true);
			return true;
			}
        iTk = start;
	}
    rule_end("typeBase", false);
	return false;
}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl() {
    rule_start("arrayDecl");
    Token *start = iTk;
    bool success = false;
    if (consume(LBRACKET)) {
        consume(INT); // optional INT literal for size
        if (consume(RBRACKET)) {
            success = true;
        } else {
            tkerr("missing ] after [ in array declaration");
        }
    }

    if (!success) {
         iTk = start;
    }
    rule_end("arrayDecl", success);
    return success;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef() {
    rule_start("varDef");
    Token *start = iTk;
    bool success = false;
    if (typeBase()) {
        if (consume(ID)) {
            arrayDecl(); // optional array declaration
            if (consume(SEMICOLON)) {
                success = true;
            } else {
                tkerr("missing ; after variable definition");
            }
        } else {
            tkerr("missing identifier in variable definition");
        }
    }

    if (!success) {
         iTk = start;
    }
    rule_end("varDef", success);
    return success;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef() {
    rule_start("structDef");
    Token *start = iTk;
    bool success = false;
    if (consume(STRUCT)) {
        if (consume(ID)) {
            if (consume(LACC)) {
                while (varDef()) {}
                if (consume(RACC)) {
                    if (consume(SEMICOLON)) {
                        success = true;
                    } else {
                        tkerr("missing ; after struct definition");
                    }
                } else {
                    tkerr("missing } in struct definition");
                }
            } else {
                 iTk = start;
            }
        } else {
            iTk = start;
        }
    }
    rule_end("structDef", success);
    return success;
}

// fnParam: typeBase ID arrayDecl?
bool fnParam() {
    rule_start("fnParam");
    Token *start = iTk;
    bool success = false;
    if (typeBase()) {
        if (consume(ID)) {
            arrayDecl(); // optional
            success = true;
        } else {
            iTk = start;
        }
    }
    rule_end("fnParam", success);
    return success;
}

// fnDef: (typeBase | VOID) ID LPAR (fnParam (COMMA fnParam)*)? RPAR stmCompound
bool fnDef() {
    rule_start("fnDef");
    Token *start = iTk;
    bool success = false;
    bool isTypeBase = typeBase();

    if (isTypeBase || consume(VOID)) {
        if (consume(ID)) {
            if (consume(LPAR)) {
                if (fnParam()) { // Optional first parameter
                    while (consume(COMMA)) {
                        if (!fnParam()) {
                            tkerr("missing or invalid function parameter after comma");
                        }
                    }
                }
                if (consume(RPAR)) {
                    if (stmCompound()) {
                        success = true;
                    } else {
                         tkerr("missing compound statement (function body)");
                    }
                } else {
                    tkerr("missing ) after function parameters");
                }
            } else {
                iTk = start;
            }
        } else {
             iTk = start;
        }
    }
    rule_end("fnDef", success);
    return success;
}

// stmCompound: LACC (varDef | stm)* RACC
bool stmCompound() {
    rule_start("stmCompound");
    bool success = false;
    if (consume(LACC)) {
        for (;;) {
            if (varDef()) {}
            else if (stm()) {}
            else break;
        }
        if (consume(RACC)) {
            success = true;
        } else {
            tkerr("missing } in compound statement");
        }
    }
    rule_end("stmCompound", success);
    return success;
}

// stm: stmCompound | IF LPAR expr RPAR stm (ELSE stm)? | WHILE LPAR expr RPAR stm | RETURN expr? SEMICOLON | expr? SEMICOLON
bool stm() {
    rule_start("stm");
    Token *start = iTk;

    if (stmCompound()) { rule_end("stm", true); return true; }

    if (consume(IF)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) {
                        if (consume(ELSE)) {
                            if (!stm()) tkerr("missing statement after else");
                        }
                        rule_end("stm", true); return true;
                    } else tkerr("missing statement after if (...)");
                } else tkerr("missing ) after if condition");
            } else tkerr("invalid or missing expression in if condition");
        } else tkerr("missing ( after if");
    }

    if (consume(WHILE)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) { rule_end("stm", true); return true; }
                    tkerr("missing statement after while (...)");
                } else tkerr("missing ) after while condition");
            } else tkerr("invalid or missing expression in while condition");
        } else tkerr("missing ( after while");
    }

    if (consume(RETURN)) {
        expr(); 
        if (consume(SEMICOLON)) { rule_end("stm", true); return true; }
        tkerr("missing ; after return statement");
    }

    expr(); 
    if (consume(SEMICOLON)) {
        rule_end("stm", true); return true;
    }

    iTk = start;
    rule_end("stm", false);
    return false;
}

// expr: exprAssign
bool expr() {
    rule_start("expr");
    bool success = exprAssign();
    rule_end("expr", success);
    return success;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign() {
    rule_start("exprAssign");
    Token *start = iTk;

    if (exprUnary()) {
        if (consume(ASSIGN)) {
            if (exprAssign()) {
                 rule_end("exprAssign", true);
                 return true;
            }
            tkerr("invalid or missing expression after =");
        }
        iTk = start;
    } else {
        
    }


    iTk = start; 
    if (exprOr()) {
        rule_end("exprAssign", true);
        return true;
    }

    rule_end("exprAssign", false);
    return false;
}


// exprOr: exprAnd exprOrPrim
bool exprOr() {
    rule_start("exprOr");
    Token *start = iTk;
    bool success = false;
    if (exprAnd()) {
        if (exprOrPrim()) {
             success = true;
        }
    }
    if (!success) iTk = start;
    rule_end("exprOr", success);
    return success;
}

// exprOrPrim: OR exprAnd exprOrPrim | epsilon
bool exprOrPrim() {
    rule_start("exprOrPrim");
    if (consume(OR)) {
        if (exprAnd()) {
            if (exprOrPrim()) {
                 rule_end("exprOrPrim", true);
                 return true;
            }
        }
        tkerr("invalid expression after || operator");
    }
    rule_end("exprOrPrim", true);
    return true;
}

// exprAnd: exprEq exprAndPrim
bool exprAnd() {
    rule_start("exprAnd");
    Token *start = iTk;
    bool success = false;
    if (exprEq()) {
        if (exprAndPrim()) {
            success = true;
        }
    }
    if (!success) iTk = start;
    rule_end("exprAnd", success);
    return success;
}

// exprAndPrim: AND exprEq exprAndPrim | epsilon
bool exprAndPrim() {
    rule_start("exprAndPrim");
    if (consume(AND)) {
        if (exprEq()) {
            if (exprAndPrim()) {
                rule_end("exprAndPrim", true);
                return true;
            }
        }
        tkerr("invalid expression after && operator");
    }
    rule_end("exprAndPrim", true); 
    return true;
}

// exprEq: exprRel exprEqPrim
bool exprEq() {
    rule_start("exprEq");
    Token *start = iTk;
    bool success = false;
    if (exprRel()) {
        if (exprEqPrim()) {
             success = true;
        }
    }
     if (!success) iTk = start;
    rule_end("exprEq", success);
    return success;
}

// exprEqPrim: (EQUAL | NOTEQ) exprRel exprEqPrim | epsilon
bool exprEqPrim() {
    rule_start("exprEqPrim");
    bool consumedOp = consume(EQUAL) || consume(NOTEQ);
    if (consumedOp) {
        if (exprRel()) {
            if (exprEqPrim()) {
                 rule_end("exprEqPrim", true);
                 return true;
            }
        }
        tkerr("invalid expression after equality operator (== or !=)");
    }
    rule_end("exprEqPrim", true); 
    return true;
}

// exprRel: exprAdd exprRelPrim
bool exprRel() {
    rule_start("exprRel");
    Token *start = iTk;
    bool success = false;
    if (exprAdd()) {
        if (exprRelPrim()) {
            success = true;
        }
    }
    if (!success) iTk = start;
    rule_end("exprRel", success);
    return success;
}

// exprRelPrim: (LESS | LESSEQ | GREATER | GREATEREQ) exprAdd exprRelPrim | epsilon
bool exprRelPrim() {
    rule_start("exprRelPrim");
    bool consumedOp = consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ);
    if (consumedOp) {
        if (exprAdd()) {
            if (exprRelPrim()) {
                rule_end("exprRelPrim", true);
                return true;
            }
        }
        tkerr("invalid expression after relational operator (<, <=, >, >=)");
    }
    rule_end("exprRelPrim", true);
    return true;
}

// exprAdd: exprMul exprAddPrim
bool exprAdd() {
    rule_start("exprAdd");
    Token *start = iTk;
    bool success = false;
    if (exprMul()) {
        if (exprAddPrim()) {
            success = true;
        }
    }
    if (!success) iTk = start;
    rule_end("exprAdd", success);
    return success;
}

// exprAddPrim: (ADD | SUB) exprMul exprAddPrim | epsilon
bool exprAddPrim() {
    rule_start("exprAddPrim");
    bool consumedOp = consume(ADD) || consume(SUB);
    if (consumedOp) {
        if (exprMul()) {
            if (exprAddPrim()) {
                 rule_end("exprAddPrim", true);
                 return true;
            }
        }
         tkerr("invalid expression after additive operator (+ or -)");
    }
    rule_end("exprAddPrim", true);
    return true;
}

// exprMul: exprCast exprMulPrim
bool exprMul() {
    rule_start("exprMul");
    Token *start = iTk;
    bool success = false;
    if (exprCast()) {
        if (exprMulPrim()) {
             success = true;
        }
    }
    if (!success) iTk = start;
    rule_end("exprMul", success);
    return success;
}

// exprMulPrim: (MUL | DIV) exprCast exprMulPrim | epsilon
bool exprMulPrim() {
    rule_start("exprMulPrim");
    bool consumedOp = consume(MUL) || consume(DIV);
    if (consumedOp) {
        if (exprCast()) {
            if (exprMulPrim()) {
                rule_end("exprMulPrim", true);
                return true;
            }
        }
         tkerr("invalid expression after multiplicative operator (* or /)");
    }
    rule_end("exprMulPrim", true); 
    return true;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast() {
    rule_start("exprCast");
    Token *start = iTk;
    bool success = false;
    if (consume(LPAR)) {
        if (typeBase()) {
            arrayDecl(); // Optional
            if (consume(RPAR)) {
                if (exprCast()) {
                     success = true;
                } else {
                     tkerr("invalid expression after type cast");
                }
            } else {
                iTk = start;
            }
        } else {
             iTk = start;
        }
        if (success) {
             rule_end("exprCast", true);
             return true;
        }
    }

    if (exprUnary()) {
        rule_end("exprCast", true);
        return true;
    }

    rule_end("exprCast", false);
    return false;
}

// exprUnary: (SUB | NOT) exprUnary | exprPostfix
bool exprUnary() {
    rule_start("exprUnary");
    bool consumedOp = consume(SUB) || consume(NOT);
    if (consumedOp) {
        if (exprUnary()) {
             rule_end("exprUnary", true);
             return true;
        }
         tkerr("invalid expression after unary operator (- or !)");
    }

    if (exprPostfix()) {
        rule_end("exprUnary", true);
        return true;
    }

    rule_end("exprUnary", false);
    return false;
}


// exprPostfix: exprPrimary exprPostfixPrim
bool exprPostfix() {
    rule_start("exprPostfix");
    bool success = false;
    if (exprPrimary()) {
        if (exprPostfixPrim()) {
            success = true;
        }
    }
    rule_end("exprPostfix", success);
    return success;
}

// exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | epsilon
bool exprPostfixPrim() {
    rule_start("exprPostfixPrim");

    if (consume(LBRACKET)) {
        if (expr()) {
            if (consume(RBRACKET)) {
                if (exprPostfixPrim()) {
                     rule_end("exprPostfixPrim", true);
                     return true;
                }
                 rule_end("exprPostfixPrim", true);
                 return true;
            } else {
                tkerr("missing ] after array index expression");
            }
        } else {
            tkerr("invalid or missing expression for array index");
        }
    }

    if (consume(DOT)) {
        if (consume(ID)) {
            if (exprPostfixPrim()) {
                rule_end("exprPostfixPrim", true);
                return true;
            }
            rule_end("exprPostfixPrim", true);
            return true;
        } else {
             tkerr("missing identifier after . operator");
        }
    }

    rule_end("exprPostfixPrim", true); // Epsilon is success
    return true;
}


// exprPrimary: ID (LPAR (expr (COMMA expr)*)? RPAR)? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary() {
    rule_start("exprPrimary");

    if (consume(ID)) {
        if (consume(LPAR)) {
            if (expr()) {
                while (consume(COMMA)) {
                    if (!expr()) {
                        tkerr("invalid or missing expression after , in function call");
                    }
                }
            }
            if (consume(RPAR)) {
                 rule_end("exprPrimary", true);
                 return true;
            }
            tkerr("missing ) after function call arguments");
        }
        rule_end("exprPrimary", true);
        return true;
    }

    if (consume(INT)) { rule_end("exprPrimary", true); return true; }
    if (consume(DOUBLE)) { rule_end("exprPrimary", true); return true; }
    if (consume(CHAR)) { rule_end("exprPrimary", true); return true; }
    if (consume(STRING)) { rule_end("exprPrimary", true); return true; }

    if (consume(LPAR)) {
        if (expr()) {
            if (consume(RPAR)) {
                 rule_end("exprPrimary", true);
                 return true;
            }
            tkerr("missing ) after expression in parentheses");
        } else {
            tkerr("invalid or missing expression after (");
        }
    }

    rule_end("exprPrimary", false);
    return false;
}

// unit: (structDef | fnDef | varDef)* END
bool unit() {
    rule_start("unit");
    for (;;) {
        if (structDef()) continue;
        if (fnDef()) continue;
        if (varDef()) continue;
        break;
    }
    if (consume(END)) {
        rule_end("unit", true);
        return true;
    }
    tkerr("syntax error or unexpected token at the end of the program");
    rule_end("unit", false); // Unreachable
    return false;
}

void parse(Token *tokens) {
    initParserLog(); // Initialize the log file
    iTk = tokens;
    if (!unit()) {
        fprintf(stderr, "Syntax analysis failed at top level.\n");
        // Optionally log the failure to the file too
        if (parser_log_fp) fprintf(parser_log_fp, "Syntax analysis failed at top level.\n"); 
    }
    closeParserLog(); // Close the log file
}