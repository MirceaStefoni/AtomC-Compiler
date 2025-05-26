#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "ad.h" 
#include "at.h"
#include "utils.h" 
#include "gc.h"

FILE *parser_log_fp = NULL;
Symbol *owner = NULL;

void initParserLog() {
    parser_log_fp = fopen("results/parser.txt", "w");
    if (parser_log_fp == NULL) {
        perror("Error opening results/parser.txt");
        exit(EXIT_FAILURE);
    }
}

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

Token *iTk;
Token *consumedTk;

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

bool stm();
bool expr(Ret *r);
bool typeBase(Type *t);
bool arrayDecl(Type *t);
bool stmCompound(bool newDomain);

bool typeBase(Type *t){
    rule_start("typeBase");
    Token *start = iTk;
    t->n = -1;

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
            Token *tkName = consumedTk;
            t->tb = TB_STRUCT;
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
    iTk = start;
    rule_end("typeBase", false);
    return false;
}

bool arrayDecl(Type *t) {
    rule_start("arrayDecl");
    if (consume(LBRACKET)) {
        if (consume(INT)) {
            Token *tkSize = consumedTk;
            if (tkSize->i < 0) tkerr("array size cannot be negative: %d", tkSize->i);
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

bool varDef() {
    rule_start("varDef");
    Token *start = iTk;
    Type t;

    if (typeBase(&t)) {
        if (consume(ID)) {
            Token *tkName = consumedTk;
            arrayDecl(&t);

            if (t.n == 0 && owner == NULL) {
                 tkerr("a global array variable must have a specified dimension: %s[]", tkName->text);
            }
            if (t.n > 0 && t.tb == TB_VOID) {
                tkerr("cannot declare array of void: %s", tkName->text);
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
                        if (t.n == 0) tkerr("struct field array must have dimension: %s[]", tkName->text);
                        if (t.tb == TB_VOID) tkerr("struct field cannot be void: %s", tkName->text);
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
                 if (size <= 0 && t.n != 0 ) {
                     tkerr("cannot allocate variable with size <= 0: %s", tkName->text);
                 }
                 if (t.n >=0) {
                    var->varMem = safeAlloc(size);
                    memset(var->varMem, 0, size);
                 } else {
                    var->varMem = safeAlloc(size);
                    memset(var->varMem, 0, size);
                 }
            }

            if (consume(SEMICOLON)) {
                rule_end("varDef", true);
                return true;
            } else {
                tkerr("missing ; after variable definition for %s", tkName->text);
            }
        } else {
            iTk = start;
        }
    }

    iTk = start;
    rule_end("varDef", false);
    return false;
}

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
                s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
                s->type.tb = TB_STRUCT;
                s->type.s = s;
                s->type.n = -1;

                Symbol *crtOwner = owner;
                owner = s;
                pushDomain();

                if (consume(LACC)) {
                    while (varDef()) {}
                    if (consume(RACC)) {
                        dropDomain();
                        owner = crtOwner;
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
    return false;
}

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
            if (t.tb == TB_VOID && t.n < 0) tkerr("function parameter cannot be void: %s", tkName->text);

            Symbol *param = findSymbolInDomain(symTable, tkName->text);
            if (param) {
                tkerr("symbol redefinition in function parameters: %s", tkName->text);
            }

            param = newSymbol(tkName->text, SK_PARAM);
            param->type = t;
            param->owner = owner;

            if (!owner || owner->kind != SK_FN) {
                 tkerr("Internal error: fnParam called outside of function definition context");
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
    return false;
}

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
                        addInstr(&fn->fn.instr, OP_ENTER);
                        if (stmCompound(false)) {
                            fn->fn.instr->arg.i = symbolsLen(fn->fn.locals);
                            if (fn->type.tb == TB_VOID) {
                                addInstrWithInt(&fn->fn.instr, OP_RET_VOID, symbolsLen(fn->fn.params));
                            } else {
                                addInstrWithInt(&fn->fn.instr, OP_RET, symbolsLen(fn->fn.params));
                            }
                            dropDomain();
                            owner = crtOwner;
                            rule_end("fnDef", true);
                            return true;
                        } else {
                            dropDomain();
                            owner = crtOwner;
                            tkerr("missing or invalid compound statement (function body) for %s", tkName->text);
                        }
                    } else {
                        dropDomain(); owner = crtOwner;
                        tkerr("missing ) after function parameters for %s", tkName->text);
                    }
                } else {
                     dropDomain(); owner = crtOwner;
                    tkerr("Internal error: LPAR expected but not found after lookahead in fnDef");
                }
            } else {
                iTk = start;
                rule_end("fnDef", false);
                return false;
            }
        } else {
            iTk = start;
            rule_end("fnDef", false);
            return false;
        }
    }
    rule_end("fnDef", false);
    return false;
}

bool stmCompound(bool newDomain) {
    rule_start("stmCompound");
    Token* start_token_stmCompound = iTk;
    bool domain_pushed_by_this_call = false;

    if (consume(LACC)) {
        if (newDomain) {
            pushDomain();
            domain_pushed_by_this_call = true;
        }

        for (;;) {
            Token* before_stm_or_vardef = iTk;
            if (varDef()) {}
            else if (stm()) {}
            else {
                if (iTk != before_stm_or_vardef) {
                    tkerr("Syntax error within compound statement near line %d", iTk->line);
                }
                break;
            }
        }

        if (consume(RACC)) {
            if (domain_pushed_by_this_call) {
                dropDomain();
            }
            rule_end("stmCompound", true);
            return true;
        } else {
            if (domain_pushed_by_this_call) dropDomain();
            tkerr("missing } in compound statement, started on line %d", start_token_stmCompound->line);
        }
    }

    iTk = start_token_stmCompound;
    rule_end("stmCompound", false);
    return false;
}

bool stm() {
    rule_start("stm");
    Token *start = iTk;
    Ret rCond, rExprRet, rExprStmt;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if (stmCompound(true)) {
        rule_end("stm", true);
        return true;
    }

    if (consume(IF)) {
        if (consume(LPAR)) {
            if (expr(&rCond)) {
                if (!canBeScalar(&rCond)) tkerr("the if condition must be a scalar value");
                addRVal(&owner->fn.instr, rCond.lval, &rCond.type);
                Type intType = {TB_INT, NULL, -1};
                insertConvIfNeeded(lastInstr(owner->fn.instr), &rCond.type, &intType);
                Instr *ifJF = addInstr(&owner->fn.instr, OP_JF);
                if (consume(RPAR)) {
                    if (stm()) {
                        if (consume(ELSE)) {
                            Instr *ifJMP = addInstr(&owner->fn.instr, OP_JMP);
                            ifJF->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
                            if (!stm()) tkerr("missing statement after else");
                            ifJMP->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
                        } else {
                            ifJF->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
                        }
                        rule_end("stm", true); return true;
                    } else tkerr("missing statement after if (...)");
                } else tkerr("missing ) after if condition");
            } else tkerr("invalid or missing expression in if condition");
        } else tkerr("missing ( after if");
        iTk = start;
        if (owner) delInstrAfter(startInstr);
        rule_end("stm", false); return false;
    }

    if (consume(WHILE)) {
        Instr *beforeWhileCond = lastInstr(owner->fn.instr);
        if (consume(LPAR)) {
            if (expr(&rCond)) {
                if (!canBeScalar(&rCond)) tkerr("the while condition must be a scalar value");
                addRVal(&owner->fn.instr, rCond.lval, &rCond.type);
                Type intType = {TB_INT, NULL, -1};
                insertConvIfNeeded(lastInstr(owner->fn.instr), &rCond.type, &intType);
                Instr *whileJF = addInstr(&owner->fn.instr, OP_JF);
                if (consume(RPAR)) {
                    if (stm()) {
                        addInstr(&owner->fn.instr, OP_JMP)->arg.instr = beforeWhileCond->next;
                        whileJF->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
                        rule_end("stm", true); return true;
                    } else tkerr("missing statement after while (...)");
                } else tkerr("missing ) after while condition");
            } else tkerr("invalid or missing expression in while condition");
        } else tkerr("missing ( after while");
        iTk = start;
        if (owner) delInstrAfter(startInstr);
        rule_end("stm", false); return false;
    }

    if (consume(RETURN)) {
        Token* returnToken = consumedTk;
        Instr *lastBeforeExpr = lastInstr(owner->fn.instr);
        if (iTk->code != SEMICOLON) {
             if(expr(&rExprRet)) {
                 addRVal(&owner->fn.instr, rExprRet.lval, &rExprRet.type);
                 insertConvIfNeeded(lastInstr(owner->fn.instr), &rExprRet.type, &owner->type);
                 addInstrWithInt(&owner->fn.instr, OP_RET, symbolsLen(owner->fn.params));
             } else {
                 tkerr("invalid expression in return statement on line %d", returnToken->line);
             }
        } else {
            if (owner->type.tb != TB_VOID) {
                tkerr("a non-void function must return a value (line %d)", returnToken->line);
            }
            addInstr(&owner->fn.instr, OP_RET_VOID);
        }
        if (consume(SEMICOLON)) { rule_end("stm", true); return true; }
        else tkerr("missing ; after return statement on line %d", returnToken->line);
        iTk = start;
        if (owner) delInstrAfter(lastBeforeExpr);
        rule_end("stm", false); return false;
    }

    Token* beforeExprStmt = iTk;
    Instr *lastBeforeExpr = lastInstr(owner->fn.instr);
    if (iTk->code != SEMICOLON && expr(&rExprStmt)) {
        if (rExprStmt.type.tb != TB_VOID) {
            addInstr(&owner->fn.instr, OP_DROP);
        }
        if (consume(SEMICOLON)) {
            rule_end("stm", true); return true;
        } else {
            tkerr("missing ; after expression statement on line %d", consumedTk->line);
        }
    } else {
        iTk = beforeExprStmt;
        if (owner) delInstrAfter(lastBeforeExpr);
        if (consume(SEMICOLON)) {
            rule_end("stm", true); return true;
        }
    }

    iTk = start;
    if (owner) delInstrAfter(startInstr);
    rule_end("stm", false);
    return false;
}

bool expr(Ret *r) {
    rule_start("expr");
    bool success = exprAssign(r);
    rule_end("expr", success);
    return success;
}

bool exprAssign(Ret *r) {
    rule_start("exprAssign");
    Token *start_assign = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    Ret rDst;

    if (exprUnary(&rDst)) {
        if (consume(ASSIGN)) {
            if (exprAssign(r)) {
                if (!rDst.lval) tkerr("the assign destination must be a left-value");
                if (rDst.ct) tkerr("the assign destination cannot be constant");
                if (!canBeScalar(&rDst)) tkerr("the assign destination must be scalar");
                if (!canBeScalar(r)) tkerr("the assign source must be scalar");
                if (!convTo(&r->type, &rDst.type)) tkerr("the assign source cannot be converted to destination");

                addRVal(&owner->fn.instr, r->lval, &r->type);
                insertConvIfNeeded(lastInstr(owner->fn.instr), &r->type, &rDst.type);
                switch(rDst.type.tb) {
                    case TB_INT: addInstr(&owner->fn.instr, OP_STORE_I); break;
                    case TB_DOUBLE: addInstr(&owner->fn.instr, OP_STORE_F); break;
                    case TB_CHAR: tkerr("assignment to char type not supported"); break;
                    case TB_VOID: tkerr("cannot assign to void type"); break;
                    case TB_STRUCT: tkerr("cannot assign to struct type"); break;
                }
                r->lval = false;
                r->ct = true;
                rule_end("exprAssign", true);
                return true;
            } else {
                tkerr("invalid or missing expression after =");
            }
        }
        iTk = start_assign;
        if (owner) delInstrAfter(startInstr);
    }

    if (exprOr(r)) {
        rule_end("exprAssign", true);
        return true;
    }

    rule_end("exprAssign", false);
    return false;
}

bool exprOr(Ret *r) {
    rule_start("exprOr");
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    if (exprAnd(r)) {
        if (exprOrPrim(r)) {
             rule_end("exprOr", true);
             return true;
        }
    }
    iTk = start;
    if (owner) delInstrAfter(startInstr);
    rule_end("exprOr", false);
    return false;
}

bool exprOrPrim(Ret *r) {
    rule_start("exprOrPrim");
    if (consume(OR)) {
        Ret right;
        if (!exprAnd(&right)) {
            tkerr("expresie lipsa dupa ||");
            return false;
        }

        Type tDst;
        if (!arithTypeTo(&r->type, &right.type, &tDst)) {
            tkerr("invalid operand types for ||");
        }
        *r = (Ret){{TB_INT, NULL, -1}, false, true};
        return exprOrPrim(r);
    }
    rule_end("exprOrPrim", true);
    return true;
}

bool exprAnd(Ret *r) {
    rule_start("exprAnd");
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    if (exprEq(r)) {
        if (exprAndPrim(r)) {
            rule_end("exprAnd", true);
            return true;
        }
    }
    iTk = start;
    if (owner) delInstrAfter(startInstr);
    rule_end("exprAnd", false);
    return false;
}

bool exprAndPrim(Ret *r) {
    rule_start("exprAndPrim");
    if (consume(AND)) {
        Ret right;
        if (!exprAnd(&right)) {
            tkerr("expresie lipsa dupa &&");
            return false;
        }

        Type tDst;
        if (!arithTypeTo(&r->type, &right.type, &tDst)) {
            tkerr("invalid operand types for &&");
        }
        *r = (Ret){{TB_INT, NULL, -1}, false, true};
        return exprAndPrim(r);
    }
    rule_end("exprAndPrim", true);
    return true;
}

bool exprEq(Ret *r) {
    rule_start("exprEq");
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    if (exprRel(r)) {
        if (exprEqPrim(r)) {
             rule_end("exprEq", true);
             return true;
        }
    }
    iTk = start;
    if (owner) delInstrAfter(startInstr);
    rule_end("exprEq", false);
    return false;
}

bool exprEqPrim(Ret *r) {
    rule_start("exprEqPrim");
    Token* opTk = iTk;
    if (consume(EQUAL) || consume(NOTEQ)) {
        Ret right;
        if (exprRel(&right)) {
            if(!arithTypeTo(&r->type, &right.type, &r->type)) tkerr("invalid operand types for %s", opTk->code == EQUAL ? "==":"!=");

            *r = (Ret){{TB_INT, NULL, -1}, false, r->ct && right.ct};
            if (exprEqPrim(r)) {
                 rule_end("exprEqPrim", true);
                 return true;
            }
        } else {
            tkerr("invalid expression after equality operator '%s'", opTk->code == EQUAL ? "==":"!=");
            return false;
        }
    }
    rule_end("exprEqPrim", true);
    return true;
}

bool exprRel(Ret *r) {
    rule_start("exprRel");
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    if (exprAdd(r)) {
        if (exprRelPrim(r)) {
            rule_end("exprRel", true);
            return true;
        }
    }
    iTk = start;
    if (owner) delInstrAfter(startInstr);
    rule_end("exprRel", false);
    return false;
}

bool exprRelPrim(Ret *r) {
    rule_start("exprRelPrim");
    Token* opTk = iTk;
    if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
        Ret right;
        char opName[3];
        if (opTk->code == LESS) strcpy(opName, "<");
        else if (opTk->code == LESSEQ) strcpy(opName, "<=");
        else if (opTk->code == GREATER) strcpy(opName, ">");
        else strcpy(opName, ">=");

        Instr *lastLeft = lastInstr(owner->fn.instr);
        addRVal(&owner->fn.instr, r->lval, &r->type);
        if (exprAdd(&right)) {
            addRVal(&owner->fn.instr, right.lval, &right.type);
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand types for %s", opName);
            insertConvIfNeeded(lastLeft, &r->type, &tDst);
            insertConvIfNeeded(lastInstr(owner->fn.instr), &right.type, &tDst);
            switch(opTk->code) {
                case LESS:
                    switch(tDst.tb) {
                        case TB_INT: addInstr(&owner->fn.instr, OP_LESS_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_LESS_F); break;
                        case TB_CHAR: tkerr("relational operator < not supported for char"); break;
                        case TB_VOID: tkerr("relational operator < not supported for void"); break;
                        case TB_STRUCT: tkerr("relational operator < not supported for struct"); break;
                    }
                    break;
                case LESSEQ:
                    switch(tDst.tb) {
                        case TB_INT: addInstr(&owner->fn.instr, OP_LESSEQ_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_LESSEQ_F); break;
                        case TB_CHAR: tkerr("relational operator <= not supported for char"); break;
                        case TB_VOID: tkerr("relational operator <= not supported for void"); break;
                        case TB_STRUCT: tkerr("relational operator <= not supported for struct"); break;
                    }
                    break;
                case GREATER:
                    switch(tDst.tb) {
                        case TB_INT: addInstr(&owner->fn.instr, OP_GREATER_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_GREATER_F); break;
                        case TB_CHAR: tkerr("relational operator > not supported for char"); break;
                        case TB_VOID: tkerr("relational operator > not supported for void"); break;
                        case TB_STRUCT: tkerr("relational operator > not supported for struct"); break;
                    }
                    break;
                case GREATEREQ:
                    switch(tDst.tb) {
                        case TB_INT: addInstr(&owner->fn.instr, OP_GREATEREQ_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_GREATEREQ_F); break;
                        case TB_CHAR: tkerr("relational operator >= not supported for char"); break;
                        case TB_VOID: tkerr("relational operator >= not supported for void"); break;
                        case TB_STRUCT: tkerr("relational operator >= not supported for struct"); break;
                    }
                    break;
            }
            *r = (Ret){{TB_INT, NULL, -1}, false, true};
            if (exprRelPrim(r)) {
                 rule_end("exprRelPrim", true);
                 return true;
            }
        } else {
            tkerr("invalid expression after relational operator '%s'", opName);
            return false;
        }
    }
    rule_end("exprRelPrim", true);
    return true;
}

bool exprAdd(Ret *r) {
    rule_start("exprAdd");
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    if (exprMul(r)) {
        if (exprAddPrim(r)) {
            rule_end("exprAdd", true);
            return true;
        }
    }
    iTk = start;
    if (owner) delInstrAfter(startInstr);
    rule_end("exprAdd", false);
    return false;
}

bool exprAddPrim(Ret *r) {
    rule_start("exprAddPrim");
    Token* opTk = iTk;
    if (consume(ADD) || consume(SUB)) {
        Ret right;
        Instr *lastLeft = lastInstr(owner->fn.instr);
        addRVal(&owner->fn.instr, r->lval, &r->type);
        if (exprMul(&right)) {
            addRVal(&owner->fn.instr, right.lval, &right.type);
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand types for %s", opTk->code == ADD ? "+":"-");
            insertConvIfNeeded(lastLeft, &r->type, &tDst);
            insertConvIfNeeded(lastInstr(owner->fn.instr), &right.type, &tDst);
            switch(opTk->code) {
                case ADD:
                    switch(tDst.tb) {
                        case TB_INT: addInstr(&owner->fn.instr, OP_ADD_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_ADD_F); break;
                        case TB_CHAR: tkerr("addition not supported for char"); break;
                        case TB_VOID: tkerr("addition not supported for void"); break;
                        case TB_STRUCT: tkerr("addition not supported for struct"); break;
                    }
                    break;
                case SUB:
                    switch(tDst.tb) {
                        case TB_INT: addInstr(&owner->fn.instr, OP_SUB_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_SUB_F); break;
                        case TB_CHAR: tkerr("subtraction not supported for char"); break;
                        case TB_VOID: tkerr("subtraction not supported for void"); break;
                        case TB_STRUCT: tkerr("subtraction not supported for struct"); break;
                    }
                    break;
            }
            *r = (Ret){tDst, false, true};
            if (exprAddPrim(r)) {
                 rule_end("exprAddPrim", true);
                 return true;
            }
        } else {
            tkerr("invalid expression after additive operator '%s'", opTk->code == ADD ? "+":"-");
            return false;
        }
    }
    rule_end("exprAddPrim", true);
    return true;
}

bool exprMul(Ret *r) {
    rule_start("exprMul");
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    if (exprCast(r)) {
        if (exprMulPrim(r)) {
             rule_end("exprMul", true);
             return true;
        }
    }
    iTk = start;
    if (owner) delInstrAfter(startInstr);
    rule_end("exprMul", false);
    return false;
}

bool exprMulPrim(Ret *r) {
    rule_start("exprMulPrim");
    Token* opTk = iTk;
    if (consume(MUL) || consume(DIV)) {
        Ret right;
        Instr *lastLeft = lastInstr(owner->fn.instr);
        addRVal(&owner->fn.instr, r->lval, &r->type);
        if (exprCast(&right)) {
            addRVal(&owner->fn.instr, right.lval, &right.type);
            Type tDst;
            if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand types for %s", opTk->code == MUL ? "*":"/");
            insertConvIfNeeded(lastLeft, &r->type, &tDst);
            insertConvIfNeeded(lastInstr(owner->fn.instr), &right.type, &tDst);
            switch(opTk->code) {
                case MUL:
                    switch(tDst.tb) {
                        case TB_INT: addInstr(&owner->fn.instr, OP_MUL_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_MUL_F); break;
                        case TB_CHAR: tkerr("multiplication not supported for char"); break;
                        case TB_VOID: tkerr("multiplication not supported for void"); break;
                        case TB_STRUCT: tkerr("multiplication not supported for struct"); break;
                    }
                    break;
                case DIV:
                    switch(tDst.tb) {
                        case TB_INT: addInstr(&owner->fn.instr, OP_DIV_I); break;
                        case TB_DOUBLE: addInstr(&owner->fn.instr, OP_DIV_F); break;
                        case TB_CHAR: tkerr("division not supported for char"); break;
                        case TB_VOID: tkerr("division not supported for void"); break;
                        case TB_STRUCT: tkerr("division not supported for struct"); break;
                    }
                    break;
            }
            *r = (Ret){tDst, false, true};
            if (exprMulPrim(r)) {
                rule_end("exprMulPrim", true);
                return true;
            }
        } else {
            tkerr("invalid expression after multiplicative operator '%s'", opTk->code == MUL ? "*":"/");
            return false;
        }
    }
    rule_end("exprMulPrim", true);
    return true;
}

bool exprCast(Ret *r) {
    rule_start("exprCast");
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if (consume(LPAR)) {
        Type t;
        Ret op;
        Token* LPARtoken = consumedTk;
        if (typeBase(&t)) {
             arrayDecl(&t);
            if (consume(RPAR)) {
                if (exprCast(&op)) {
                    if(t.tb == TB_STRUCT) tkerr("cannot convert to a struct type");
                    if(op.type.tb == TB_STRUCT) tkerr("cannot convert a struct");
                    if(op.type.n >= 0 && t.n < 0) tkerr("an array can be converted only to another array");
                    if(op.type.n < 0 && t.n >= 0) tkerr("a scalar can be converted only to another scalar");

                    *r = (Ret){t, false, op.ct};
                    rule_end("exprCast", true);
                    return true;
                } else {
                     tkerr("invalid expression after type cast");
                }
            } else {
                 tkerr("missing ) in type cast expression, started on line %d", LPARtoken->line);
            }
        }
        iTk = start;
        if (owner) delInstrAfter(startInstr);
    }

    if (exprUnary(r)) {
        rule_end("exprCast", true);
        return true;
    }

    rule_end("exprCast", false);
    return false;
}

bool exprUnary(Ret *r) {
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    if (consume(SUB)) {
        if (exprUnary(r)) {
            if (!canBeScalar(r))
                tkerr("unary - must have a scalar operand");
            r->lval = false;
            r->ct = true;
            return true;
        } else {
            tkerr("expresie invalida dupa operatorul unar -");
        }
    } else if (consume(NOT)) {
        if (exprUnary(r)) {
            if (!canBeScalar(r))
                tkerr("unary ! must have a scalar operand");
            r->lval = false;
            r->ct = true;
            r->type = (Type){TB_INT, NULL, -1};
            return true;
        } else {
            tkerr("expresie invalida dupa operatorul unar !");
        }
    }
    if (exprPostfix(r)) {
        return true;
    }
    if (owner) delInstrAfter(startInstr);
    return false;
}

bool exprPostfix(Ret *r) {
    rule_start("exprPostfix");
    Token* start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
    if (exprPrimary(r)) {
        if (exprPostfixPrim(r)) {
            rule_end("exprPostfix", true);
            return true;
        }
    }
    iTk = start;
    if (owner) delInstrAfter(startInstr);
    rule_end("exprPostfix", false);
    return false;
}

bool exprPostfixPrim(Ret *r) {
    rule_start("exprPostfixPrim");
    Token* LBracketOrDotToken = iTk;

    if (consume(LBRACKET)) {
        Ret idx;
        if (expr(&idx)) {
            if (consume(RBRACKET)) {
                if(r->type.n < 0) tkerr("only an array can be indexed");
                Type tInt={TB_INT, NULL, -1};
                if(!convTo(&idx.type, &tInt)) tkerr("the index is not convertible to int");

                r->type.n = -1;
                r->lval = true;
                r->ct = false;

                if (exprPostfixPrim(r)) {
                    rule_end("exprPostfixPrim", true);
                    return true;
                }
            } else {
                tkerr("missing ] after array index expression, started on line %d", LBracketOrDotToken->line);
            }
        } else {
            tkerr("invalid or missing expression for array index, started on line %d", LBracketOrDotToken->line);
        }
        return false;
    }

    if (consume(DOT)) {
        if (consume(ID)) {
            Token* tkName = consumedTk;
            if(r->type.tb != TB_STRUCT) tkerr("a field can only be selected from a struct");
            Symbol *s_field = findSymbolInList(r->type.s->structMembers, tkName->text);
            if(!s_field) tkerr("the structure %s does not have a field %s", r->type.s->name, tkName->text);
            *r = (Ret){s_field->type, true, (s_field->type.n >= 0)};

            if (exprPostfixPrim(r)) {
                rule_end("exprPostfixPrim", true);
                return true;
            }
        } else {
             tkerr("missing identifier after . operator on line %d", LBracketOrDotToken->line);
        }
        return false;
    }

    rule_end("exprPostfixPrim", true);
    return true;
}

bool exprPrimary(Ret *r) {
    rule_start("exprPrimary");
    Token *start = iTk;
    Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;

    if (consume(ID)) {
        Token *tkName = consumedTk;
        Symbol *s = findSymbol(tkName->text);
        if (!s)
            tkerr("undefined id: %s", tkName->text);

        if (consume(LPAR)) {
            if (s->kind != SK_FN)
                tkerr("only a function can be called");
            Symbol *param = s->fn.params;
            Ret rArg;
            int paramCount = 0;
            if (iTk->code != RPAR) {
                do {
                    if (!expr(&rArg))
                        tkerr("invalid expression for argument in function call %s", tkName->text);
                    if (!param)
                        tkerr("too many arguments in function call %s", tkName->text);
                    if (!convTo(&rArg.type, &param->type))
                        tkerr("in call to %s, cannot convert argument type to parameter type", tkName->text);
                    addRVal(&owner->fn.instr, rArg.lval, &rArg.type);
                    insertConvIfNeeded(lastInstr(owner->fn.instr), &rArg.type, &param->type);
                    param = param->next;
                    paramCount++;
                } while (consume(COMMA));
            }
            if (param)
                tkerr("too few arguments in function call %s", tkName->text);
            if (!consume(RPAR))
                tkerr("lipseste ) dupa argumentele functiei %s", tkName->text);
            if (s->fn.extFnPtr) {
                addInstr(&owner->fn.instr, OP_CALL_EXT)->arg.extFnPtr = s->fn.extFnPtr;
            } else {
                addInstr(&owner->fn.instr, OP_CALL)->arg.instr = s->fn.instr;
            }
            *r = (Ret){s->type, false, true};
            rule_end("exprPrimary", true);
            return true;
        }
        if (s->kind == SK_FN)
            tkerr("function %s cannot be used as a value", tkName->text);
        if (s->kind == SK_VAR) {
            if (s->owner == NULL) {
                addInstr(&owner->fn.instr, OP_ADDR)->arg.p = s->varMem;
            } else {
                switch (s->type.tb) {
                    case TB_INT: addInstrWithInt(&owner->fn.instr, OP_FPADDR_I, s->varIdx + 1); break;
                    case TB_DOUBLE: addInstrWithInt(&owner->fn.instr, OP_FPADDR_F, s->varIdx + 1); break;
                    case TB_CHAR: tkerr("variable access not supported for char"); break;
                    case TB_VOID: tkerr("variable access not supported for void"); break;
                    case TB_STRUCT: tkerr("variable access not supported for struct"); break;
                }
            }
        }
        if (s->kind == SK_PARAM)
        {
            int nParams = symbolsLen(s->owner->fn.params);
            int offset = -(nParams - s->paramIdx + 1);
            switch (s->type.tb)
            {
            case TB_INT:
                addInstrWithInt(&owner->fn.instr, OP_FPADDR_I, offset);
                break;
            case TB_DOUBLE:
                addInstrWithInt(&owner->fn.instr, OP_FPADDR_F, offset);
                break;
            case TB_CHAR:
                tkerr("parameter access not supported for char");
                break;
            case TB_VOID:
                tkerr("parameter access not supported for void");
                break;
            case TB_STRUCT:
                tkerr("parameter access not supported for struct");
                break;
            }
        }
        *r = (Ret){s->type, true, (s->type.n >= 0)};
        rule_end("exprPrimary", true);
        return true;
    }

    if (consume(INT)) {
        Token *ct = consumedTk;
        addInstrWithInt(&owner->fn.instr, OP_PUSH_I, ct->i);
        *r = (Ret){{TB_INT, NULL, -1}, false, true};
        rule_end("exprPrimary", true);
        return true;
    }
    if (consume(DOUBLE)) {
        Token *ct = consumedTk;
        addInstrWithDouble(&owner->fn.instr, OP_PUSH_F, ct->d);
        *r = (Ret){{TB_DOUBLE, NULL, -1}, false, true};
        rule_end("exprPrimary", true);
        return true;
    }
    if (consume(LPAR)) {
        if (iTk->code == TYPE_INT || iTk->code == TYPE_DOUBLE ||
            iTk->code == TYPE_CHAR || iTk->code == STRUCT) {
            iTk = start;
        } else {
            if (expr(r)) {
                if (!consume(RPAR))
                    tkerr("lipseste ) dupa expresie in paranteze");
                rule_end("exprPrimary", true);
                return true;
            } else {
                tkerr("expresie invalida dupa (");
            }
        }
    }
    iTk = start;
    if (owner) delInstrAfter(startInstr);
    rule_end("exprPrimary", false);
    return false;
}

bool unit() {
    rule_start("unit");
    for (;;) {
        Token* beforeDef = iTk;
        if (structDef()) continue;
        if (fnDef()) continue;
        if (varDef()) continue;

        if (iTk->code == END) break;

        if (iTk == beforeDef) {
             tkerr("syntax error or unexpected token '%s' at top level or end of file", tkCodeName(iTk->code));
             break;
        }
    }

    if (consume(END)) {
        rule_end("unit", true);
        return true;
    }

    tkerr("syntax error or unexpected token '%s' at the end of the program or top level, expected END", tkCodeName(iTk->code));
    rule_end("unit", false);
    return false;
}

void parse(Token *tokens_list) {
    initParserLog();
    initAdLog();
    iTk = tokens_list;

    if (!unit()) {
        fprintf(stderr, "Syntax analysis failed.\n");
        closeAdLog();
        closeParserLog();
        exit(EXIT_FAILURE);
    }

    printf("Domain analysis and Type analysis completed successfully.\n");
    showDomain(symTable, "global");

    closeAdLog();
    closeParserLog();
}