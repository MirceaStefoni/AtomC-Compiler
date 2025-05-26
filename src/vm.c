#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "ad.h"

Instr *addInstr(Instr **list,Opcode op){
    Instr *i=(Instr*)safeAlloc(sizeof(Instr));
    i->op=op;
    i->next=NULL;
    if(*list){
        Instr *p=*list;
        while(p->next)p=p->next;
        p->next=i;
        }else{
        *list=i;
        }
    return i;
    }

Instr *insertInstr(Instr *before,int op){
    Instr *i=(Instr*)safeAlloc(sizeof(Instr));
    i->op=op;
    i->next=before->next;
    before->next=i;
    return i;
    }

void delInstrAfter(Instr *instr){
    if(!instr)return;
    for(Instr *next=instr->next,*i=next;i;i=next){
        next=i->next;
        free(i);
        }
    instr->next=NULL;
}

Instr *lastInstr(Instr *list){
    if(list){
        while(list->next)list=list->next;
        }
    return list;
    }

Instr *addInstrWithInt(Instr **list,Opcode op,int argVal){
    Instr *i=addInstr(list,op);
    i->arg.i=argVal;
    return i;
    }

Instr *addInstrWithDouble(Instr **list,Opcode op,double argVal){
    Instr *i=addInstr(list,op);
    i->arg.f=argVal;
    return i;
    }

#define MAXSTACK 10000
Val stack[MAXSTACK];
Val *SP=stack-1;
Val *FP=NULL;

void pushv(Val v){
    if(SP+1==stack+MAXSTACK)err("trying to push into a full stack");
    *++SP=v;
    }

Val popv(){
    if(SP==stack-1)err("trying to pop from empty stack");
    return *SP--;
    }

void pushi(int i){
    if(SP+1==stack+MAXSTACK)err("trying to push into a full stack");
    (++SP)->i=i;
    }

int popi(){
    if(SP==stack-1)err("trying to pop from empty stack");
    return SP--->i;
    }

double popf(){
    if(SP==stack-1)err("trying to pop from empty stack");
    return SP--->f;
    }

void pushf(double f){
    if(SP+1==stack+MAXSTACK)err("trying to push into a full stack");
    (++SP)->f=f;
    }

void pushp(void *p){
    if(SP+1==stack+MAXSTACK)err("trying to push into a full stack");
    (++SP)->p=p;
    }

void *popp(){
    if(SP==stack-1)err("trying to pop from empty stack");
    return SP--->p;
    }

void put_i(){
    printf("=> %d",popi());
    }

void vmInit(){
    Symbol *fn=addExtFn("put_i",put_i,(Type){TB_VOID,NULL,-1});
    addFnParam(fn,"i",(Type){TB_INT,NULL,-1});
    }

void run(Instr *IP){
    Val v;
    int iArg,iTop,iBefore;
    double fTop,fBefore;
    void *pTop;
    void(*extFnPtr)();
    for(;;){
        printf("%p/%d\t",IP,(int)(SP-stack+1));
        switch(IP->op){
            case OP_HALT:
                printf("HALT");
                return;
            case OP_PUSH_I:
                printf("PUSH.i\t%d",IP->arg.i);
                pushi(IP->arg.i);
                IP=IP->next;
                break;
            case OP_CALL:
                pushp(IP->next);
                printf("CALL\t%p",IP->arg.instr);
                IP=IP->arg.instr;
                break;
            case OP_CALL_EXT:
                extFnPtr=IP->arg.extFnPtr;
                printf("CALL_EXT\t%p\n",extFnPtr);
                extFnPtr();
                IP=IP->next;
                break;
            case OP_ENTER:
                pushp(FP);
                FP=SP;
                SP+=IP->arg.i;
                printf("ENTER\t%d",IP->arg.i);
                IP=IP->next;
                break;
            case OP_RET_VOID:
                iArg=IP->arg.i;
                printf("RET_VOID\t%d",iArg);
                IP=FP[-1].p;
                SP=FP-iArg-2;
                FP=FP[0].p;
                break;
            case OP_JMP:
                printf("JMP\t%p",IP->arg.instr);
                IP=IP->arg.instr;
                break;
            case OP_JF:
                iTop=popi();
                printf("JF\t%p\t// %d",IP->arg.instr,iTop);
                IP=iTop ? IP->next : IP->arg.instr;
                break;
            case OP_FPLOAD:
                v=FP[IP->arg.i];
                pushv(v);
                printf("FPLOAD\t%d\t// i:%d, f:%g",IP->arg.i,v.i,v.f);
                IP=IP->next;
                break;
            case OP_FPSTORE:
                v=popv();
                FP[IP->arg.i]=v;
                printf("FPSTORE\t%d\t// i:%d, f:%g",IP->arg.i,v.i,v.f);
                IP=IP->next;
                break;
            case OP_ADD_I:
                iTop=popi();
                iBefore=popi();
                pushi(iBefore+iTop);
                printf("ADD.i\t// %d+%d -> %d",iBefore,iTop,iBefore+iTop);
                IP=IP->next;
                break;
            case OP_LESS_I:
                iTop=popi();
                iBefore=popi();
                pushi(iBefore<iTop);
                printf("LESS.i\t// %d<%d -> %d",iBefore,iTop,iBefore<iTop);
                IP=IP->next;
                break;
            case OP_CONV_F_I:
                fTop=popf();
                pushi((int)fTop);
                printf("CONV.f.i\t// %g -> %d",fTop,(int)fTop);
                IP=IP->next;
                break;
            case OP_DROP:
                popv();
                printf("DROP");
                IP=IP->next;
                break;
            case OP_PUSH_F:
                printf("PUSH.f\t%g",IP->arg.f);
                pushf(IP->arg.f);
                IP=IP->next;
                break;
            case OP_FPADDR_I:
                pTop=&FP[IP->arg.i].i;
                pushp(pTop);
                printf("FPADDR\t%d\t// %p",IP->arg.i,pTop);
                IP=IP->next;
                break;
            case OP_LOAD_I:
                pTop=popp();
                pushi(*(int*)pTop);
                printf("LOAD.i\t// *(int*)%p -> %d",pTop,*(int*)pTop);
                IP=IP->next;
                break;
            case OP_NOP:
                printf("NOP");
                IP=IP->next;
                break;
            case OP_RET:
                v=popv();
                iArg=IP->arg.i;
                printf("RET\t%d\t// i:%d, f:%g",iArg,v.i,v.f);
                IP=FP[-1].p;
                SP=FP-iArg-2;
                FP=FP[0].p;
                pushv(v);
                break;
            case OP_SUB_I:
                iTop=popi();
                iBefore=popi();
                pushi(iBefore-iTop);
                printf("SUB.i\t// %d-%d -> %d",iBefore,iTop,iBefore-iTop);
                IP=IP->next;
                break;
            case OP_MUL_I:
                iTop=popi();
                iBefore=popi();
                pushi(iBefore*iTop);
                printf("MUL.i\t// %d*%d -> %d",iBefore,iTop,iBefore*iTop);
                IP=IP->next;
                break;
            case OP_STORE_I:
                iTop=popi();
                v=popv();
                *(int*)v.p=iTop;
                pushi(iTop);
                printf("STORE.i\t// *(int*)%p=%d",v.p,iTop);
                IP=IP->next;
                break;
            case OP_ADD_F:
                fTop=popf();
                fBefore=popf();
                pushf(fBefore+fTop);
                printf("ADD.f\t// %g+%g -> %g",fBefore,fTop,fBefore+fTop);
                IP=IP->next;
                break;
            case OP_SUB_F:
                fTop=popf();
                fBefore=popf();
                pushf(fBefore-fTop);
                printf("SUB.f\t// %g-%g -> %g",fBefore,fTop,fBefore-fTop);
                IP=IP->next;
                break;
            case OP_MUL_F:
                fTop=popf();
                fBefore=popf();
                pushf(fBefore*fTop);
                printf("MUL.f\t// %g*%g -> %g",fBefore,fTop,fBefore*fTop);
                IP=IP->next;
                break;
            case OP_DIV_I:
                iTop=popi();
                iBefore=popi();
                if(iTop==0)err("division by zero");
                pushi(iBefore/iTop);
                printf("DIV.i\t// %d/%d -> %d",iBefore,iTop,iBefore/iTop);
                IP=IP->next;
                break;
            case OP_DIV_F:
                fTop=popf();
                fBefore=popf();
                if(fTop==0.0)err("division by zero");
                pushf(fBefore/fTop);
                printf("DIV.f\t// %g/%g -> %g",fBefore,fTop,fBefore/fTop);
                IP=IP->next;
                break;
            case OP_LESS_F:
                fTop=popf();
                fBefore=popf();
                pushi(fBefore<fTop);
                printf("LESS.f\t// %g<%g -> %d",fBefore,fTop,fBefore<fTop);
                IP=IP->next;
                break;
            case OP_LESSEQ_I:
                iTop=popi();
                iBefore=popi();
                pushi(iBefore<=iTop);
                printf("LESSEQ.i\t// %d<=%d -> %d",iBefore,iTop,iBefore<=iTop);
                IP=IP->next;
                break;
            case OP_LESSEQ_F:
                fTop=popf();
                fBefore=popf();
                pushi(fBefore<=fTop);
                printf("LESSEQ.f\t// %g<=%g -> %d",fBefore,fTop,fBefore<=fTop);
                IP=IP->next;
                break;
            case OP_GREATER_I:
                iTop=popi();
                iBefore=popi();
                pushi(iBefore>iTop);
                printf("GREATER.i\t// %d>%d -> %d",iBefore,iTop,iBefore>iTop);
                IP=IP->next;
                break;
            case OP_GREATER_F:
                fTop=popf();
                fBefore=popf();
                pushi(fBefore>fTop);
                printf("GREATER.f\t// %g>%g -> %d",fBefore,fTop,fBefore>fTop);
                IP=IP->next;
                break;
            case OP_GREATEREQ_I:
                iTop=popi();
                iBefore=popi();
                pushi(iBefore>=iTop);
                printf("GREATEREQ.i\t// %d>=%d -> %d",iBefore,iTop,iBefore>=iTop);
                IP=IP->next;
                break;
            case OP_GREATEREQ_F:
                fTop=popf();
                fBefore=popf();
                pushi(fBefore>=fTop);
                printf("GREATEREQ.f\t// %g>=%g -> %d",fBefore,fTop,fBefore>=fTop);
                IP=IP->next;
                break;
            case OP_LOAD_F:
                pTop=popp();
                pushf(*(double*)pTop);
                printf("LOAD.f\t// *(double*)%p -> %g",pTop,*(double*)pTop);
                IP=IP->next;
                break;
            case OP_STORE_F:
                fTop=popf();
                v=popv();
                *(double*)v.p=fTop;
                pushf(fTop);
                printf("STORE.f\t// *(double*)%p=%g",v.p,fTop);
                IP=IP->next;
                break;
            case OP_CONV_I_F:
                iTop=popi();
                pushf((double)iTop);
                printf("CONV.i.f\t// %d -> %g",iTop,(double)iTop);
                IP=IP->next;
                break;
            default:err("run: not implemented instruction: %d",IP->op);
            }
        putchar('\n');
        }
    }

Instr *genTestProgram(){
    Instr *code=NULL;
    addInstrWithInt(&code,OP_PUSH_I,2);
    Instr *callPos=addInstr(&code,OP_CALL);
    addInstr(&code,OP_HALT);
    callPos->arg.instr=addInstrWithInt(&code,OP_ENTER,1);
    addInstrWithInt(&code,OP_PUSH_I,0);
    addInstrWithInt(&code,OP_FPSTORE,1);
    Instr *whilePos=addInstrWithInt(&code,OP_FPLOAD,1);
    addInstrWithInt(&code,OP_FPLOAD,-2);
    addInstr(&code,OP_LESS_I);
    Instr *jfAfter=addInstr(&code,OP_JF);
    addInstrWithInt(&code,OP_FPLOAD,1);
    Symbol *s=findSymbol("put_i");
    if(!s)err("undefined: put_i");
    addInstr(&code,OP_CALL_EXT)->arg.extFnPtr=s->fn.extFnPtr;
    addInstrWithInt(&code,OP_FPLOAD,1);
    addInstrWithInt(&code,OP_PUSH_I,1);
    addInstr(&code,OP_ADD_I);
    addInstrWithInt(&code,OP_FPSTORE,1);
    addInstr(&code,OP_JMP)->arg.instr=whilePos;
    jfAfter->arg.instr=addInstrWithInt(&code,OP_RET_VOID,1);
    return code;
    }