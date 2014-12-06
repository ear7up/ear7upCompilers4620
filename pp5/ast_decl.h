/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp5: You will need to extend the Decl classes to implement 
 * code generation for declarations.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "list.h"
#include "ast_type.h"

class Type;
class NamedType;
class Identifier;
class Stmt;
class InterfaceDecl;

class Decl : public Node 
{
  protected:
    Identifier *id;
  
  public:
    Decl(Identifier *name);
    const char* GetName() { return id->GetName(); }
    friend std::ostream& operator<<(std::ostream& out, Decl *d) { return out << d->id; }
    Identifier *GetId() { return id; }
    Type* GetType() { return Type::errorType; }
    bool IsMethodDecl() { return false; }
};

class VarDecl : public Decl 
{
  protected:
    Type *type;
    
  public:
    VarDecl(Identifier *name, Type *type);
    Type* GetType() { return type; }
    void Emit(Scope* parentScope);
};

class ClassDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    NamedType *extends;
    List<NamedType*> *implements;
    List<InterfaceDecl*> *imp_decls;

  public:
    ClassDecl(Identifier *name, NamedType *extends, 
              List<NamedType*> *implements, List<Decl*> *members);
    Scope* PrepareScope();
};

class InterfaceDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    
  public:
    InterfaceDecl(Identifier *name, List<Decl*> *members);
    Scope* PrepareScope();
};

class FnDecl : public Decl 
{
  protected:
    List<VarDecl*> *formals;
    Type *returnType;
    Stmt *body;
    
  public:
    FnDecl(Identifier *name, Type *returnType, List<VarDecl*> *formals);
    void SetFunctionBody(Stmt *b);
    virtual void Emit(Scope* parentScope);
    bool ConflictsWithPrevious(Decl*);
    bool MatchesPrototype(FnDecl* prototype);
    bool IsMethodDecl();
};

#endif
