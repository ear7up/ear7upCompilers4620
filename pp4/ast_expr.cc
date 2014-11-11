/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>

#include "errors.h"

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

void CompoundExpr::Check()
{
    if (right)
    {
	right->Check();
	right->GetType();
    }
    if (left)
    {
	left->Check();
	left->GetType();
    }
}
   
  
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

void ArrayAccess::Check()
{
    base->Check();
    subscript->Check();
    GetType();
}

Type* ArrayAccess::GetType()
{
    if (!type)
    {
        ArrayType *at = dynamic_cast<ArrayType*>(base->GetType());
	if (!at)
        {
            ReportError::BracketsOnNonArray(base);
	    type = Type::errorType;
        }
        else if (!subscript->GetType()->IsEquivalentTo(Type::intType))
        {
            ReportError::SubscriptNotInteger(subscript);
	    type = Type::errorType;
        } 
    }
    return type;
}
    
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

void FieldAccess::Check() {
    Type* ftype = GetType();
}

Type* FieldAccess::GetType()
{
    // if we know the type, return it, otherwise derive it from the base
    if (type) return type;
    if (base)
    {
        base->Check();
	Type* base_type = base->GetType();
	NamedType* base_ntype = dynamic_cast<NamedType*>(base_type);
	// if the base has a named type, i.e., Base.field
	if (base_ntype)
	{
    	    Decl* base_decl = base_ntype->GetDeclForType();
	    if (base_decl)
	    {
		// If the access refers to an unknown field in the Base, report that error
	        Decl* field_decl = base_decl->FindDecl(field);
		if (!field_decl)
		{
		    ReportError::FieldNotFoundInBase(field, base_type);
		    type = Type::errorType;
		    return type;
		}
		// If we can't find the field's declaration in the current scope, report the class it is accessible from
		else if (!this->FindDecl(field))
		{
		    ReportError::InaccessibleField(field, base_type);
	            type = Type::errorType;
		    return type;
		}
	    }
	}
	// report an error if we are not using a proper base (not a named type)
	else
	{
	    ReportError::FieldNotFoundInBase(field, base_type);
	    type = Type::errorType;
	    return type;
	}
    }
    else
    {
	Decl* decl = parent->FindDecl(field);
        if (!decl)
	{
	    ReportError::IdentifierNotDeclared(field, LookingForVariable);
	    type = Type::errorType;
   	    return type;
        }
    }
    VarDecl* variable_decl = dynamic_cast<VarDecl*>(FindDecl(field));
    if (variable_decl)
	type = variable_decl->GetDeclaredType();
    else
	type = Type::errorType;
    return type;
}


Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

Type* Call::GetType()
{
    return Type::errorType; // not done :(
}

void Call::Check()
{
    // not done :(
}


NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}

Type* NewExpr::GetType()
{
    if (!type)
    {
        // must be able to find the class type of the object we create
	if (cType and !dynamic_cast<ClassDecl*>(FindDecl(cType->GetId())))
        {
            type = Type::errorType;
	    ReportError::IdentifierNotDeclared(cType->GetId(), LookingForClass);
        }
	else { type = cType; }
    }
    return type;
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

Type* NewArrayExpr::GetType()
{
    return type;
}

// check to see if 'this' is used inside of a class function
void This::Check() {
    GetType();
}

// get the cType of the parent
Type* This::GetType() {
    Node* node = GetParent();
    while (node) {
        ClassDecl* parent_class = dynamic_cast<ClassDecl*>(node);
        if (parent_class) {
            type = parent_class->cType;
            break;
        }
        node = node->GetParent();
    }
    // if we can't find a parent class, we have an error: this outside of a class
    if (!type) {
        ReportError::ThisOutsideClassScope(this);
        type = Type::errorType;
    }
    return type;
}

Type* ArithmeticExpr::GetType()
{
    if (!type)
    {
        if (!right or !right->GetType())
        {
	    type = Type::errorType;
	}
	else
	{
            Type *rt = right->GetType();

            // minus is the only unary arithmetic expression we use, and it must have an integer or double right operand
            if (!left and op->str()[0] == '-')  
            {
                if (rt->IsEquivalentTo(Type::intType)    or
                    rt->IsEquivalentTo(Type::doubleType) or
                    rt->IsEquivalentTo(Type::errorType))
                {
                    type = rt;
                }
                else
                {
                    type = Type::errorType;
	        }
            }
            else if (!left)
            {
	        // we are missing an operator for this operand
	        ReportError::IncompatibleOperand(op, rt);
                type = Type::errorType;
            }
            else
            {
                Type* lt = left->GetType();
	        if ((lt->IsEquivalentTo(Type::intType) and rt->IsEquivalentTo(Type::intType)) or
                (lt->IsEquivalentTo(Type::doubleType) and rt->IsEquivalentTo(Type::doubleType)))
	        {
	            type = lt;
	        }
		else // operands do not have the same type
	        {
		    type = Type::errorType;
		    ReportError::IncompatibleOperands(op, lt, rt);
		}
            }
	}
    }    
    return type;
}

Type* RelationalExpr::GetType()
{
    if (!type)
    {
        if (!left or !right)
	    type = Type::errorType;
        else
	{
	    Type *lt = left->GetType();
	    Type *rt = right->GetType();
	    // don't repeat errors
	    if (lt->IsEquivalentTo(Type::errorType) or rt->IsEquivalentTo(Type::errorType))
	        type = Type::errorType;
	    else if ( (lt->IsEquivalentTo(Type::intType) and rt->IsEquivalentTo(Type::intType)) or
		      (lt->IsEquivalentTo(Type::doubleType) and rt->IsEquivalentTo(Type::doubleType)) )
	    {
		// both int/double, return correct bool type
		type = Type::boolType;
	    }
	    else
	    {
		// mismatched types
		type = Type::errorType;
		ReportError::IncompatibleOperands(op, lt, rt);
 	    }
	}
    }
    return type;
}

Type* EqualityExpr::GetType()
{
    if (!type)
    {
        if (right and left)
        {
            Type *rt = right->GetType();
            Type *lt = left->GetType();
            if (rt->IsEquivalentTo(Type::errorType) or lt->IsEquivalentTo(Type::errorType))
                type = Type::errorType;
            else if (rt->IsEquivalentTo(Type::nullType) or
                     lt->IsEquivalentTo(Type::nullType) or
                     rt->IsEquivalentTo(lt))
                type = Type::boolType;
            else
            {
                // types are not comparable
                type = Type::errorType;
                ReportError::IncompatibleOperands(op, lt, rt);
            }
        }
        else // right hand side of equality expression cannot be null
            type = Type::errorType;
    }
    return type;
}

Type* LogicalExpr::GetType()
{
    if (!type)
    {
        if (!right)
            type = Type::errorType;
        else
        {
            Type *rt = right->GetType();
            if (!left and op->str()[0] == '!' and 
               (rt->IsEquivalentTo(Type::boolType) or
                rt->IsEquivalentTo(Type::errorType)))
                type = Type::boolType;
            else if (!left)
            {
                // left can only be null for the not operator
                type = Type::errorType;
                ReportError::IncompatibleOperand(op, rt);
            }
            else 
            {
               Type *lt = left->GetType();
               if (rt->IsEquivalentTo(Type::boolType) and lt->IsEquivalentTo(Type::boolType))
                   type = Type::boolType;
               else
               {
                   // logical operators require boolean operands
                   type = Type::errorType;
                   ReportError::IncompatibleOperands(op, lt, rt);
               }
            }
            
        }
        
    }
    return type;
}

Type* AssignExpr::GetType()
{
    if (!type)
    {
        if (!right or !left)
            type = Type::errorType;
        else
        {
            Type *rt = right->GetType();
	    Type *lt = left->GetType();
            if (rt->IsEquivalentTo(Type::nullType) or rt->IsEquivalentTo(lt))
               type = lt;
            else
            {
               // invalid assignment, different types
               type = Type::errorType;
               ReportError::IncompatibleOperands(op, lt, rt);
            }
        }
    }
    return type;   
}


