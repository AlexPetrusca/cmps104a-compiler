%{

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lyutils.h"
#include "astree.h"

%}

%debug
%defines
%error-verbose
%token-table
%verbose

%destructor { destroy ($$); } <>
%printer { astree::dump (yyoutput, $$); } <>

%initial-action {
   parser::root = new astree (TOK_ROOT, {0, 0, 0}, "");
}

%token TOK_VOID TOK_CHAR TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_NULL TOK_NEW TOK_ARRAY
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_IDENT TOK_INTCON TOK_CHARCON TOK_STRINGCON

%token TOK_BAD_IDENT TOK_BAD_CHAR TOK_BAD_STR

%token TOK_DECLID TOK_INDEX TOK_RETURNVOID TOK_VARDECL
%token TOK_BLOCK TOK_CALL TOK_IFELSE TOK_INITDECL
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID TOK_FIELD
%token TOK_ORD TOK_CHR TOK_NEWSTRING TOK_ROOT
%token TOK_FUNCTION TOK_PARAMLIST TOK_PROTOTYPE

%token '{'

%right TOK_IF
%right TOK_ELSE
%right '='
%left TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left '+' '-'
%left '*' '/' '%'
%right TOK_POS TOK_NEG '!'
%left '[' '.' '('

%start start

%%


start       : program
                { $$ = $1 = nullptr; }
            ;
program     : program structdef
                {
                    $$ = $1->adopt($2);
                    parser::root->lloc.filenr = $2->lloc.filenr;
                }
            | program function
                {
                    $$ = $1->adopt($2);
                    parser::root->lloc.filenr = $2->lloc.filenr;
                }
            | program statement
                {
                    $$ = $1->adopt($2);
                    parser::root->lloc.filenr = $2->lloc.filenr;
                }
            | program error '}'
                {
                    destroy($3); $$ = $1;
                    yyerror("error: '}'");
                }
            | program error ';'
                {
                    destroy($3);
                    $$ = $1;
                    yyerror("error: ';'");
                }
            |
                { $$ = parser::root; }
            ;
structdef   : TOK_STRUCT TOK_IDENT '{' fieldlist '}'
                {
                    destroy($3, $5);
                    $2->symbol = TOK_TYPEID;
                    $1->adopt($2);
                    vector<astree*> childs = $4->children;
                    for(size_t i = 0; i < childs.size(); i++) {
                        $1->adopt(childs[i]);
                    }
                    $$ = $1;
                }
            ;
fieldlist   : fieldlist fielddecl ';'
                {
                    destroy($3);
                    $$ = $1->adopt($2);
                }
            |
                { $$ = new astree(TOK_ORD, {0, 0, 0}, ""); }
            ;
fielddecl   : basetype TOK_ARRAY TOK_IDENT
                {
                    $3->symbol = TOK_FIELD;
                    $$ = $2->adopt($1, $3);
                }
            | basetype TOK_IDENT
                {
                    $2->symbol = TOK_FIELD;
                    $$ = $1->adopt($2);
                }
            ;
basetype    : TOK_VOID
                { $$ = $1; }
            | TOK_INT
                { $$ = $1; }
            | TOK_STRING
                { $$ = $1; }
            | TOK_IDENT
                {
                    $1->symbol = TOK_TYPEID;
                    $$ = $1;
                }
            ;
function    : identdecl '(' paramlist ')' block
                {
                    destroy($4);
                    if($5->symbol == ';') {
                        $$ = new astree(
                            TOK_PROTOTYPE, $1->lloc, "");
                    } else {
                        $$ = new astree(
                            TOK_FUNCTION, $1->lloc, "");
                    }
                    vector<astree*> childs = $3->children;
                    $2->symbol = TOK_PARAMLIST;
                    for(size_t i = 0; i < childs.size(); i++) {
                        $2->adopt(childs[i]);
                    }
                    $$->adopt($1, $2);
                    if($5->symbol != ';') {
                        $$->adopt($5);
                    }
                }
            ;
paramlist   : identdecl idecllist
                {
                    $$ = new astree(TOK_ORD, {0, 0, 0}, "");
                    $$->adopt($1);
                    vector<astree*> childs = $2->children;
                    for(size_t i = 0; i < childs.size(); i++) {
                        $$->adopt(childs[i]);
                    }
                }
            |
                { $$ = new astree(TOK_ORD, {0, 0, 0}, ""); }
            ;
idecllist   : idecllist ',' identdecl
                {
                    destroy($2);
                    $$ = $1->adopt($3);
                }
            |
                { $$ = new astree(TOK_ORD, {0, 0, 0}, ""); }
            ;
identdecl   : basetype TOK_ARRAY TOK_IDENT
                {
                    $3->symbol = TOK_DECLID;
                    $$ = $2->adopt($1, $3);
                }
            | basetype TOK_IDENT
                {
                    $2->symbol = TOK_DECLID;
                    $$ = $1->adopt($2);
                }
            ;
block       : '{' statelist '}'
                {
                    destroy($3);
                    vector<astree*> childs = $2->children;
                    $1->symbol = TOK_BLOCK;
                    for(size_t i = 0; i < childs.size(); i++) {
                        $1->adopt(childs[i]);
                    }
                    $$ = $1;
                }
            | ';'
                { $$ = $1; }
            ;
statelist   : statelist statement
                { $$ = $1->adopt($2); }
            |
                { $$ = new astree(TOK_ORD, {0, 0, 0}, ""); }
            ;
statement   : block
                { $$ = $1; }
            | vardecl
                { $$ = $1; }
            | while
                { $$ = $1; }
            | ifelse
                { $$ = $1; }
            | return
                { $$ = $1; }
            | expr ';'
                {
                    destroy($2);
                    $$ = $1;
                }
            ;
vardecl     : identdecl '=' expr ';'
                {
                    destroy($4);
                    $2->symbol = TOK_VARDECL;
                    $$ = $2->adopt($1, $3);
                }
            ;
while       : TOK_WHILE '(' expr ')' statement
                {
                    destroy($2, $4);
                    $$ = $1->adopt($3, $5);
                }
            ;
ifelse      : TOK_IF '(' expr ')' statement  %prec TOK_IF
                {
                    destroy($2, $4);
                    $$ = $1->adopt($3,$5);
                }
            | TOK_IF '(' expr ')' statement TOK_ELSE statement
                {
                    destroy($2, $4);
                    destroy($6);
                    $1->symbol = TOK_IFELSE;
                    $$ = $1->adopt($3, $5);
                    $$ = $$-> adopt($7);
                }
            ;
return      : TOK_RETURN ';'
                {
                    destroy($2);
                    $1->symbol = TOK_RETURNVOID;
                    $$ = $1;
                }
            | TOK_RETURN expr ';'
                {
                    destroy($3);
                    $$ = $1->adopt($2);
                }
            ;
expr        : expr '=' expr
                { $$ = $2->adopt($1, $3); }
            | expr TOK_EQ expr
                { $$ = $2->adopt($1, $3); }
            | expr TOK_NE expr
                { $$ = $2->adopt($1, $3); }
            | expr TOK_LT expr
                { $$ = $2->adopt($1, $3); }
            | expr TOK_LE expr
                { $$ = $2->adopt($1, $3); }
            | expr TOK_GT expr
                { $$ = $2->adopt($1, $3); }
            | expr TOK_GE expr
                { $$ = $2->adopt($1, $3); }
            | expr '+' expr
                { $$ = $2->adopt($1, $3); }
            | expr '-' expr
                { $$ = $2->adopt($1, $3); }
            | expr '*' expr
                { $$ = $2->adopt($1, $3); }
            | expr '/' expr
                { $$ = $2->adopt($1, $3); }
            | expr '%' expr
                { $$ = $2->adopt($1, $3); }
            | '+' expr  %prec TOK_POS
                { $$ = $1->adopt_sym($2, TOK_POS); }
            | '-' expr  %prec TOK_NEG
                { $$ = $1->adopt_sym($2, TOK_NEG); }
            | '!' expr
                { $$ = $1->adopt($2); }
            | allocator
                { $$ = $1; }
            | call
                { $$ = $1; }
            | '(' expr ')'
                {
                    destroy($1, $3);
                    $$ = $2;
                }
            | variable
                { $$ = $1; }
            | constant
                { $$ = $1; }
            ;
allocator   : TOK_NEW TOK_IDENT '(' ')'
                {
                    destroy($3, $4);
                    $2->symbol = TOK_TYPEID;
                    $$ = $1->adopt($2);
                }
            | TOK_NEW TOK_STRING '(' expr ')'
                {
                    destroy($2, $3);
                    destroy($5);
                    $1->symbol = TOK_NEWSTRING;
                    $$ = $1->adopt($4);
                }
            | TOK_NEW basetype '[' expr ']'
                {
                    destroy($3, $5);
                    $1->symbol = TOK_NEWARRAY;
                    $$ = $1->adopt($2, $4);
                }
            ;
call        : TOK_IDENT '(' passlist ')'
                {
                    destroy($4);
                    $2->symbol = TOK_CALL;
                    $2->adopt($1);
                    vector<astree*> childs = $3->children;
                    for(size_t i = 0; i < childs.size(); i++) {
                        $2->adopt(childs[i]);
                    }
                    $$ = $2;
                }
            ;
passlist    : expr exprlist
                {
                    $$ = new astree(TOK_ORD, {0, 0, 0}, "");
                    $$->adopt($1);
                    vector<astree*> childs = $2->children;
                    for(size_t i = 0; i < childs.size(); i++) {
                        $$->adopt(childs[i]);
                    }
                }
            |
                { $$ = new astree(TOK_ORD, {0, 0, 0}, ""); }
            ;
exprlist    : exprlist ',' expr
                {
                    destroy($2);
                    $$ = $1->adopt($3);
                }
            |
                { $$ = new astree(TOK_ORD, {0, 0, 0}, ""); }
            ;
variable    : TOK_IDENT
                { $$ = $1; }
            | expr '[' expr ']'
                {
                    destroy($4);
                    $2->symbol = TOK_INDEX;
                    $$ = $2->adopt($1, $3);
                }
            | expr '.' TOK_IDENT
                {
                    $3->symbol = TOK_FIELD;
                    $$ = $2->adopt($1, $3);
                }
            ;
constant    : TOK_INTCON
                { $$ = $1; }
            | TOK_CHARCON
                { $$ = $1; }
            | TOK_STRINGCON
                { $$ = $1; }
            | TOK_NULL
                { $$ = $1; }
            ;


%%


const char *parser::get_tname (int symbol) {
return yytname [YYTRANSLATE (symbol)];
}

bool is_defined_token (int symbol) {
return YYTRANSLATE (symbol) > YYUNDEFTOK;
}

/*
static void* yycalloc (size_t size) {
   void* result = calloc (1, size);
   assert (result != "");
   return result;
}
*/
