/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>

#include <iostream>
using namespace std;

IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}
CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}
   
  
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}
     
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}


Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}
 

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}


NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

void IntConstant::Emit(Scope* parentScope) {
    nodeScope = parentScope;
    memloc = CodeGenerator::getInstance()->GenLoadConstant(value);
}

void StringConstant::Emit(Scope* parentScope) {
    nodeScope = parentScope;
    memloc = CodeGenerator::getInstance()->GenLoadConstant(value);
}

void BoolConstant::Emit(Scope* parentScope) {
    nodeScope = parentScope;
    memloc = CodeGenerator::getInstance()->GenLoadConstant(value);
}

void CompoundExpr::Emit(Scope* parentScope) {
    nodeScope = parentScope;
    // may be recursive, let each expression be defined and a location set
    left->Emit(nodeScope);
    right->Emit(nodeScope);
    memloc = CodeGenerator::getInstance()->GenBinaryOp(op->GetOperatorString(), left->GetMemoryLocation(), right->GetMemoryLocation());
}

void AssignExpr::Emit(Scope* parentScope) {
    nodeScope = parentScope;
    CodeGenerator *codegen = CodeGenerator::getInstance();
    left->Emit(nodeScope);
    right->Emit(nodeScope);
    codegen->GenAssign(left->GetMemoryLocation(), right->GetMemoryLocation());
}

void FieldAccess::Emit(Scope* parentScope) {
    nodeScope = parentScope;
    // identifier vs. object field
    if (base) {
        // object handling
    }
    VarDecl* decl = dynamic_cast<VarDecl*>(FindDecl(field, kDeep));
    decl->Emit(nodeScope); // this decl should already have been emitted, it doesn't have memloc info, though?
    memloc = decl->GetMemoryLocation();
}

Type* ArithmeticExpr::GetType() {
    if (left->GetType()->IsEquivalentTo(Type::doubleType) or right->GetType()->IsEquivalentTo(Type::doubleType))
        return Type::doubleType;
    if (left->GetType()->IsEquivalentTo(Type::intType) and right->GetType()->IsEquivalentTo(Type::intType))
        return Type::intType;
    else return Type::errorType;
}

Type* FieldAccess::GetType() {
    return (dynamic_cast<VarDecl*>(FindDecl(field, kDeep)))->GetType();
}
      
