#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "ad.h" 
#include "utils.h" 

// Global file pointer for parser log (optional, kept from original)
FILE *parser_log_fp = NULL;

// Global pointer to the current owner symbol (function or struct)
Symbol *owner = NULL;

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

// Error reporting function
void tkerr(const char *fmt,...){
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

// Consumes the next token if it matches the expected code
bool consume(int code){
    if (parser_log_fp) fprintf(parser_log_fp, "consume(%s)", tkCodeName(code));  // optional logging
    if(iTk->code==code){
        consumedTk=iTk;
        iTk=iTk->next;
        if (parser_log_fp) fprintf(parser_log_fp, " => consumed\n");  // optional logging
        return true;
    }
    if (parser_log_fp) fprintf(parser_log_fp, " => found %s\n", tkCodeName(iTk->code));  // optional logging
    return false;
}

// Forward declarations for mutually recursive functions if necessary
bool stm();
bool expr();
bool typeBase(Type *t);
bool arrayDecl(Type *t);
bool stmCompound(bool newDomain); 

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(Type *t){
    rule_start("typeBase");
    Token *start = iTk;
    t->n = -1; // Default: not an array

	if(consume(TYPE_INT)){
        t->tb = TB_INT;
        rule_end("typeBase", true);
		return true;
	}
	if(consume(TYPE_DOUBLE)){
        t->tb = TB_DOUBLE;
        rule_end("typeBase", true);
		return true;
	}
	if(consume(TYPE_CHAR)){
        t->tb = TB_CHAR;
        rule_end("typeBase", true);
		return true;
	}
	if(consume(STRUCT)){
        if(consume(ID)){
            Token *tkName = consumedTk; // Keep track of the ID token
            t->tb = TB_STRUCT;
            // Find the struct definition globally
            t->s = findSymbol(tkName->text);
            if (!t->s) {
                tkerr("structura nedefinita: %s", tkName->text);
            }
            if (t->s->kind != SK_STRUCT) {
                 tkerr("%s is not a struct", tkName->text);
            }
            rule_end("typeBase", true);
			return true;
		} else {
             tkerr("Missing identifier after STRUCT");
        }
	}
    // No type matched
    iTk = start;
    rule_end("typeBase", false);
	return false;
}

// arrayDecl: LBRACKET (INT)? RBRACKET
bool arrayDecl(Type *t) {
    rule_start("arrayDecl");
    if (consume(LBRACKET)) {
        if (consume(INT)) {
            Token *tkSize = consumedTk;
            t->n = tkSize->i; 
        } else {
            t->n = 0;
        }
        if (consume(RBRACKET)) {
            rule_end("arrayDecl", true);
            return true;
        } else {
            tkerr("missing ] after [ in array declaration");
        }
    }
   
    rule_end("arrayDecl", false);
    return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef() {
    rule_start("varDef");
    Token *start = iTk;
    Type t; 

    if (typeBase(&t)) { 
        if (consume(ID)) {
            Token *tkName = consumedTk;      
            arrayDecl(&t);

            if (t.n == 0) {
                 tkerr("a vector variable must have a specified dimension: %s[]", tkName->text);
            }

            Symbol *var = findSymbolInDomain(symTable, tkName->text);
            if (var) {
                tkerr("symbol redefinition: %s", tkName->text);
            }

            var = newSymbol(tkName->text, SK_VAR);
            var->type = t; 
            var->owner = owner; 
            addSymbolToDomain(symTable, var); 

            if (owner) {
                switch (owner->kind) {
                    case SK_FN:

                        var->varIdx = symbolsLen(owner->fn.locals); 
                        addSymbolToList(&owner->fn.locals, dupSymbol(var)); 
                        break;
                    case SK_STRUCT:

                        var->varIdx = typeSize(&owner->type);
                        addSymbolToList(&owner->structMembers, dupSymbol(var));
                        break;
                    default:

                        tkerr("Internal error: Invalid owner kind for variable %s", tkName->text);
                        break;
                }
            } else {

                 if (t.tb == TB_VOID) tkerr("cannot declare variables of type void: %s", tkName->text);
                 int size = typeSize(&t);
                 if (size <= 0) tkerr("cannot allocate variable with size <= 0: %s", tkName->text);
                 var->varMem = safeAlloc(size);
                 memset(var->varMem, 0, size);
            }
            // --- Domain Analysis End ---


            if (consume(SEMICOLON)) {
                rule_end("varDef", true);
                return true;
            } else {
                tkerr("missing ; after variable definition for %s", tkName->text);
            }
        } else {
            tkerr("missing identifier in variable definition");
        }
    }

    iTk = start;
    rule_end("varDef", false);
    return false;
}


// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef() {
    rule_start("structDef");
    Token *start = iTk; 

    if (consume(STRUCT)) {
        if (consume(ID)) {
            Token *tkName = consumedTk; 

            if (iTk->code == LACC) {
         
                Symbol *s = findSymbolInDomain(symTable, tkName->text);
                if (s) {
                    tkerr("symbol redefinition: %s", tkName->text);
                }
                // Add the new struct symbol
                s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
                s->type.tb = TB_STRUCT;
                s->type.s = s; 
                s->type.n = -1; 

                Symbol *crtOwner = owner; 
                owner = s; 
                pushDomain(); 
                // --- Domain Analysis End ---

                if (consume(LACC)) { 
                    while (varDef()) {} 
                    if (consume(RACC)) {

                        dropDomain(); 
                        owner = crtOwner;
                        // --- Domain Analysis End ---

                        if (consume(SEMICOLON)) {
                            rule_end("structDef", true);
                            return true; 
                        } else {

                            tkerr("missing ; after struct definition for %s", tkName->text);
                        }
                    } else {
                        
                        tkerr("missing } in struct definition for %s", tkName->text);
                    }
                } else {
                     
                    tkerr("Internal error: LACC expected but not found after lookahead in structDef");
                }
            } else {

                iTk = start;
                rule_end("structDef", false);
                return false;
            }
        } else {

            iTk = start;
            rule_end("structDef", false);
            return false;
        }
    }

    rule_end("structDef", false);
    return false; // STRUCT keyword not found
}


// fnParam: typeBase ID arrayDecl?
bool fnParam() {
    rule_start("fnParam");
    Token *start = iTk;
    Type t;

    if (typeBase(&t)) { 
        if (consume(ID)) {
            Token *tkName = consumedTk;

            if (arrayDecl(&t)) {
                 t.n = 0; 
            }


            Symbol *param = findSymbolInDomain(symTable, tkName->text);
            if (param) {
                tkerr("symbol redefinition: %s", tkName->text);
            }


            param = newSymbol(tkName->text, SK_PARAM);
            param->type = t; 
            param->owner = owner; 

            if (!owner || owner->kind != SK_FN) {
                 tkerr("Internal error: fnParam called outside of function definition"); 
            }

            param->paramIdx = symbolsLen(owner->fn.params); 

    
            addSymbolToDomain(symTable, param); 
            addSymbolToList(&owner->fn.params, dupSymbol(param)); 


            rule_end("fnParam", true);
            return true;
        } else {
           
             iTk = start; 
             rule_end("fnParam", false);
             return false;
        }
    }
    rule_end("fnParam", false);
    return false; // typeBase failed
}


// fnDef: (typeBase | VOID) ID LPAR (fnParam (COMMA fnParam)*)? RPAR stmCompound
bool fnDef() {
    rule_start("fnDef");
    Token *start = iTk; 
    Type t;           
    Token *tkName = NULL; 
    bool typeFound = false;

    if (typeBase(&t)) {
        typeFound = true;
    } else if (consume(VOID)) {
        t.tb = TB_VOID;
        t.n = -1;
        t.s = NULL;
        typeFound = true;
    }

    if (typeFound) {
        if (consume(ID)) {
            tkName = consumedTk; 

            if (iTk->code == LPAR) {

                Symbol *fn = findSymbolInDomain(symTable, tkName->text);
                if (fn) {
                    tkerr("symbol redefinition: %s", tkName->text);
                }

                fn = newSymbol(tkName->text, SK_FN);
                fn->type = t;
                addSymbolToDomain(symTable, fn);

                Symbol *crtOwner = owner; 
                owner = fn; 

                pushDomain(); 

                if (consume(LPAR)) { 
   
                    if (fnParam()) {
                        while (consume(COMMA)) {
                            if (!fnParam()) {
                                tkerr("missing or invalid function parameter after comma in %s", tkName->text);
                            }
                        }
                    }
                    if (consume(RPAR)) {
                     
                        if (stmCompound(false)) { 
                          
                            dropDomain(); 
                            owner = crtOwner; 
               
                            rule_end("fnDef", true);
                            return true; 
                        } else {
                            // Error in body
                            dropDomain();
                            owner = crtOwner;
                            tkerr("missing or invalid compound statement (function body) for %s", tkName->text);
                        }
                    } else {
                        // Missing RPAR
                        dropDomain();
                        owner = crtOwner;
                        tkerr("missing ) after function parameters for %s", tkName->text);
                    }
                } else {
                    tkerr("Internal error: LPAR expected but not found after lookahead in fnDef");
                }
            } else {
            
                iTk = start;
                rule_end("fnDef", false);
                return false;
            }
        } else {
            // (typeBase | VOID) not followed by ID
            iTk = start;
            rule_end("fnDef", false);
            return false;
        }
    }

    rule_end("fnDef", false);
    return false;
}

// stmCompound: LACC (varDef | stm)* RACC
bool stmCompound(bool newDomain) {
    rule_start("stmCompound");
    Token* start = iTk; 
    bool pushed = false; 

    if (consume(LACC)) {

        if (newDomain) {
            pushDomain();
            pushed = true;
        }


        for (;;) {
            if (varDef()) {}
            else if (stm()) {}
            else break;
        }

        if (consume(RACC)) {

            if (pushed) { 
                dropDomain();
            }
        
            rule_end("stmCompound", true);
            return true;
        } else {
      
            if (pushed) dropDomain();
            tkerr("missing } in compound statement");
        }
    }

    iTk = start; 
    rule_end("stmCompound", false);
    return false;
}


// stm: stmCompound | IF LPAR expr RPAR stm (ELSE stm)? | WHILE LPAR expr RPAR stm | RETURN expr? SEMICOLON | expr? SEMICOLON
bool stm() {
    rule_start("stm");
    Token *start = iTk;

  
    if (stmCompound(true)) { 
        rule_end("stm", true);
        return true;
    }

    // IF statement
    if (consume(IF)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) { 
                        if (consume(ELSE)) { 
                            if (!stm()) { 
                                tkerr("missing statement after else");
                            }
                        }
                        rule_end("stm", true); return true; 
                    } else tkerr("missing statement after if (...)");
                } else tkerr("missing ) after if condition");
            } else tkerr("invalid or missing expression in if condition");
        } else tkerr("missing ( after if");

        iTk = start; 
        rule_end("stm", false);
        return false;
    }

    // WHILE statement
    if (consume(WHILE)) {
        if (consume(LPAR)) {
            if (expr()) {
                if (consume(RPAR)) {
                    if (stm()) { 
                        rule_end("stm", true); return true; 
                    } else tkerr("missing statement after while (...)");
                } else tkerr("missing ) after while condition");
            } else tkerr("invalid or missing expression in while condition");
        } else tkerr("missing ( after while");
        iTk = start; 
        rule_end("stm", false);
        return false;
    }

    // RETURN statement
    if (consume(RETURN)) {
        expr(); 
        
        if (consume(SEMICOLON)) {
            rule_end("stm", true); return true; 
        } else tkerr("missing ; after return statement");
         iTk = start; 
        rule_end("stm", false);
        return false;
    }

    // Expression statement (or empty statement)
    // Try parsing an expression (optional)
    bool hasExpr = expr();
    if (consume(SEMICOLON)) {
        rule_end("stm", true); return true; 
    }

    if (hasExpr) {
         tkerr("Missing ; after expression statement");
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

    if (exprOr()) { // Try the simpler path first

         iTk = start;

         if (exprUnary()) { 
             if (consume(ASSIGN)) {
                 if (exprAssign()) { 
                     rule_end("exprAssign", true);
                     return true;
                 }
                 tkerr("invalid or missing expression after =");
             }
 
             iTk = start;
         }
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
    if (exprAnd()) {
        if (exprOrPrim()) {
             rule_end("exprOr", true);
             return true;
        }
    }
    iTk = start; 
    rule_end("exprOr", false);
    return false;
}

// exprOrPrim: OR exprAnd exprOrPrim | epsilon
bool exprOrPrim() {
    rule_start("exprOrPrim");
    while (true) {
        
        if (consume(OR)) {
            if (exprAnd()) {
                continue;
            } else {
                tkerr("invalid expression after || operator");
                return false;
            }
        } else {
            break;
        }
    }
    rule_end("exprOrPrim", true);
    return true;
}

// exprAnd: exprEq exprAndPrim
bool exprAnd() {
    rule_start("exprAnd");
    Token *start = iTk;
    if (exprEq()) {
        if (exprAndPrim()) {
            rule_end("exprAnd", true);
            return true;
        }
    }
    iTk = start;
    rule_end("exprAnd", false);
    return false; 
}

// exprAndPrim: AND exprEq exprAndPrim | epsilon
bool exprAndPrim() {
    rule_start("exprAndPrim");
    while (true) {
        
        if (consume(AND)) {
            if (exprEq()) {
                continue;
            } else {
                tkerr("invalid expression after && operator");
                return false;
            }
        } else {
            break;
        }
    }
    rule_end("exprAndPrim", true);
    return true;
}

// exprEq: exprRel exprEqPrim
bool exprEq() {
    rule_start("exprEq");
    Token *start = iTk;
    if (exprRel()) {
        if (exprEqPrim()) {
             rule_end("exprEq", true);
             return true;
        }
    }
     iTk = start;
    rule_end("exprEq", false);
    return false; 
}

// exprEqPrim: (EQUAL | NOTEQ) exprRel exprEqPrim | epsilon
bool exprEqPrim() {
    rule_start("exprEqPrim");
    while (true) {
        if (consume(EQUAL)) {
            if (exprRel()) {
                continue; // Found EQUAL exprRel, continue loop
            } else {
                tkerr("invalid expression after equality operator '=='");
                return false;
            }
        } else if (consume(NOTEQ)) {
            if (exprRel()) {
                continue; // Found NOTEQ exprRel, continue loop
            } else {
                tkerr("invalid expression after equality operator '!='");
                return false;
            }
        } else {
            break; // No equality operator found, exit loop (epsilon case)
        }
    }
    rule_end("exprEqPrim", true);
    return true;
}

// exprRel: exprAdd exprRelPrim
bool exprRel() {
    rule_start("exprRel");
    Token *start = iTk;
    if (exprAdd()) {
        if (exprRelPrim()) {
            rule_end("exprRel", true);
            return true;
        }
    }
    iTk = start;
    rule_end("exprRel", false);
    return false;
}

// exprRelPrim: (LESS | LESSEQ | GREATER | GREATEREQ) exprAdd exprRelPrim | epsilon
bool exprRelPrim() {
    rule_start("exprRelPrim");
    while (true) {

        if (consume(LESS)) {
            if (exprAdd()) {
                continue; // Found LESS exprAdd, continue loop
            } else {
                 tkerr("invalid expression after relational operator '<'");
                 return false;
            }
        } else if (consume(LESSEQ)) {
             if (exprAdd()) {
                continue; // Found LESSEQ exprAdd, continue loop
            } else {
                 tkerr("invalid expression after relational operator '<='");
                 return false;
            }
        } else if (consume(GREATER)) {
             if (exprAdd()) {
                continue; // Found GREATER exprAdd, continue loop
            } else {
                 tkerr("invalid expression after relational operator '>'");
                 return false;
            }
        } else if (consume(GREATEREQ)) {
             if (exprAdd()) {
                continue; // Found GREATEREQ exprAdd, continue loop
            } else {
                 tkerr("invalid expression after relational operator '>='");
                 return false;
            }
        } else {
            break; // No relational operator found, exit loop (epsilon case)
        }
    }
    rule_end("exprRelPrim", true);
    return true;
}

// exprAdd: exprMul exprAddPrim
bool exprAdd() {
    rule_start("exprAdd");
    Token *start = iTk;
    if (exprMul()) {
        if (exprAddPrim()) {
            rule_end("exprAdd", true);
            return true;
        }
    }
    iTk = start;
    rule_end("exprAdd", false);
    return false; 
}

// exprAddPrim: (ADD | SUB) exprMul exprAddPrim | epsilon
bool exprAddPrim() {
    rule_start("exprAddPrim");
    while (true) {
        Token *opToken = iTk; // Keep track of the potential operator token
        if (consume(ADD)) {
            if (exprMul()) {
                continue; // Found ADD exprMul, continue loop
            } else {
                 tkerr("invalid expression after additive operator '%s'", opToken->text);
                 return false;
            }
        } else if (consume(SUB)) {
             if (exprMul()) {
                continue; // Found SUB exprMul, continue loop
            } else {
                 tkerr("invalid expression after additive operator '%s'", opToken->text);
                 return false;
            }
        } else {
            break; // No ADD or SUB found, exit loop (epsilon case)
        }
    }
    rule_end("exprAddPrim", true);
    return true;
}

// exprMul: exprCast exprMulPrim
bool exprMul() {
    rule_start("exprMul");
    Token *start = iTk;
    if (exprCast()) {
        if (exprMulPrim()) {
             rule_end("exprMul", true);
             return true;
        }
    }
    iTk = start;
    rule_end("exprMul", false);
    return false;
}

// exprMulPrim: (MUL | DIV) exprCast exprMulPrim | epsilon
bool exprMulPrim() {
    rule_start("exprMulPrim");
    while (true) {
        if (consume(MUL)) {
            if (exprCast()) {
                continue; // Found MUL exprCast, continue loop
            } else {
                 tkerr("invalid expression after multiplicative operator '*'");
                 return false;
            }
        } else if (consume(DIV)) {
            if (exprCast()) {
                continue; // Found DIV exprCast, continue loop
            } else {
                 tkerr("invalid expression after multiplicative operator '/'");
                 return false;
            }
        } else {
            break; // No multiplicative operator found, exit loop (epsilon case)
        }
    }
    rule_end("exprMulPrim", true);
    return true;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast() {
    rule_start("exprCast");
    Token *start = iTk;

    if (consume(LPAR)) {
        Type t; 
        if (typeBase(&t)) { 
             arrayDecl(&t); 
            if (consume(RPAR)) { 
                if (exprCast()) { 
                     rule_end("exprCast", true);
                     return true; 
                } else {
                     tkerr("invalid expression after type cast");
                     return false; 
                }
            }
        }
        iTk = start;
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
    Token* start = iTk;

    if (consume(SUB)) {
        if (exprUnary()) {
            rule_end("exprUnary", true);
            return true;
        } else {
             tkerr("invalid expression after unary operator '-'");
             return false;
        }
    } else if (consume(NOT)) {
         if (exprUnary()) {
            rule_end("exprUnary", true);
            return true;
        } else {
             tkerr("invalid expression after unary operator '!'");
             return false;
        }
    }

    if (exprPostfix()) {
        rule_end("exprUnary", true);
        return true;
    }

    iTk = start;
    rule_end("exprUnary", false);
    return false;
}


// exprPostfix: exprPrimary exprPostfixPrim
bool exprPostfix() {
    rule_start("exprPostfix");
    Token* start = iTk;
    if (exprPrimary()) {
        if (exprPostfixPrim()) { 
            rule_end("exprPostfix", true);
            return true;
        }
    }
    iTk = start; 
    rule_end("exprPostfix", false);
    return false;
}

// exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | epsilon
bool exprPostfixPrim() {
    rule_start("exprPostfixPrim");

    while (true) {
        if (consume(LBRACKET)) {
            if (expr()) {
                if (consume(RBRACKET)) {
                    // Successfully parsed '[expr]', continue loop
                    continue;
                } else {
                    tkerr("missing ] after array index expression");
                    return false;
                }
            } else {
                tkerr("invalid or missing expression for array index");
                return false;
            }
        } else if (consume(DOT)) {
            if (consume(ID)) {
                // Successfully parsed '.ID', continue loop
                continue;
            } else {
                 tkerr("missing identifier after . operator");
                 return false;
            }
        } else {
            // No more postfix operators found, break the loop
            break;
        }
    }

    rule_end("exprPostfixPrim", true);
    return true;
}


// exprPrimary: ID (LPAR (expr (COMMA expr)*)? RPAR)? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary() {
    rule_start("exprPrimary");
    Token* start = iTk;

    if (consume(ID)) {
         Token* tkName = consumedTk; 
        if (consume(LPAR)) { 
            if (expr()) { 
                while (consume(COMMA)) { 
                    if (!expr()) {
                        tkerr("invalid or missing expression after , in function call to %s", tkName->text);
                        return false; 
                    }
                }
            }
            if (consume(RPAR)) {
                 rule_end("exprPrimary", true);
                 return true; 
            }
            tkerr("missing ) after function call arguments for %s", tkName->text);
            return false; 
        }
        rule_end("exprPrimary", true);
        return true;
    }

    // Literals
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
            return false; // Error
        } else {
            tkerr("invalid or missing expression after (");
            return false; // Error
        }
    }

    iTk = start; 
    rule_end("exprPrimary", false);
    return false;
}

// unit: (structDef | fnDef | varDef)* END
bool unit() {
    rule_start("unit");
    for (;;) {
        Token* beforeDef = iTk; 
        if (structDef()) continue;
        if (fnDef()) continue;
        if (varDef()) continue;
        if (iTk == beforeDef) {
             break; 
        }
    }

    if (consume(END)) {
        rule_end("unit", true);
        return true; 
    }

    tkerr("syntax error or unexpected token '%s' at the end of the program or top level", iTk->text ? iTk->text : "unknown token");
    rule_end("unit", false); 
    return false;
}

// Main parse function
void parse(Token *tokens) {
    initParserLog(); 
    initAdLog();    
    iTk = tokens;

  
    pushDomain();

    if (!unit()) {
  
        fprintf(stderr, "Syntax analysis failed.\n");

        dropDomain(); 
        closeAdLog();   
        closeParserLog();
        exit(EXIT_FAILURE); // Exit on failure
    }

    // Analysis was successful if unit() returned true
    printf("Domain analysis completed successfully.\n"); 
    showDomain(symTable, "global"); 
    dropDomain();

    closeAdLog();     
    closeParserLog(); 
}