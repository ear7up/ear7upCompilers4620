/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "tac.h"
#include "errors.h"

#include "codegen.h"
CodeGenerator* CodeGenerator::codegen;

#include <iostream>
using namespace std;


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
    nodeScope = new Scope();
    decls->DeclareAll(nodeScope);
}

void Program::Check() {
    // Assume valid programs
}
void Program::Emit(Scope* parentScope) {
    nodeScope = new Scope();
    for (int n = 0; n < decls->NumElements(); n++) {
        decls->Nth(n)->Emit(nodeScope);
    }
    if (!hasMain) ReportError::NoMainFound();
    else CodeGenerator::getInstance()->DoFinalCodeGen();
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::Emit(Scope* parentScope) {
    nodeScope = new Scope();
    decls->DeclareAll(nodeScope);
    // emit each decl and statement
    for (int n = 0; n < stmts->NumElements(); n++)
        stmts->Nth(n)->Emit(nodeScope);
    for (int n2 = 0; n2 < decls->NumElements(); n2++)
        decls->Nth(n2)->Emit(nodeScope);
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}


ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}
  
PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void PrintStmt::Emit(Scope* parentScope) {
    nodeScope = parentScope;
    // generate loads for argument expressions to put params on the stack
    CodeGenerator *codegen = CodeGenerator::getInstance();

    // location is established by constant type's emit function
    for (int n = 0; n < args->NumElements(); n++) {
        args->Nth(n)->Emit(parentScope);
        Location *arg = args->Nth(n)->GetMemoryLocation();
        Type* arg_type = args->Nth(n)->GetType();

        // built in call handles push/pop
        if (arg_type->IsEquivalentTo(Type::intType)) {
            Location *l = codegen->GenBuiltInCall(PrintInt, arg, NULL);
        }
        else if (arg_type->IsEquivalentTo(Type::stringType)) {
            Location *l = codegen->GenBuiltInCall(PrintString, arg, NULL);
        }
        else if (arg_type->IsEquivalentTo(Type::boolType)) {
            Location *l = codegen->GenBuiltInCall(PrintBool, arg, NULL);
        }
        else {
            cout << "Error: No known type for '" << arg_type << "'" << endl;
        }
        // perform system call to appropriate Print type
    }
}

void IfStmt::Emit(Scope* parentScope) {
    nodeScope = new Scope();
    CodeGenerator *codegen = CodeGenerator::getInstance();
    char* else_label = codegen->NewLabel();
    test->Emit(nodeScope); // store test expression in a temporary variable
    codegen->GenIfZ(test->GetMemoryLocation(), else_label); // branch to else label if the test is false/zero (skip the body)
    if (body) body->Emit(nodeScope); 
    char* after_label = codegen->NewLabel();
    codegen->GenGoto(after_label); // skip over the else body if test was true
    codegen->GenLabel(else_label);
    if (elseBody) elseBody->Emit(new Scope());
    codegen->GenLabel(after_label);
}

void WhileStmt::Emit(Scope* parentScope) {
    nodeScope = new Scope();
    CodeGenerator *codegen = CodeGenerator::getInstance();
    char* before_label = codegen->NewLabel();
    char* after_label = codegen->NewLabel();
    codegen->GenLabel(before_label);
    test->Emit(nodeScope); // store test expression in a temporary variable
    codegen->GenIfZ(test->GetMemoryLocation(), after_label); // branch to else label if the test is false/zero (skip the body)
    if (body) body->Emit(nodeScope); 
    codegen->GenGoto(before_label); // skip over the body if test was true
    codegen->GenLabel(after_label);
}

void StmtBlock::Declare(Scope* scope) {
    decls->DeclareAll(scope);
}




