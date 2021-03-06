/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf

Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
    nodeScope = NULL;
}

Decl *Node::FindDecl(Identifier *id, lookup l) {
    Decl *local;
    if (!nodeScope)
        PrepareScope();
    local = nodeScope->Lookup(id);
    if (nodeScope and local)
        return local;
    if (l == kDeep and parent)
        return parent->FindDecl(id, l);  
    return NULL;
}
	 
Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
} 

