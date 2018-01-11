/* $Id: lexer.l,v 1.7 2016-10-27 19:45:59-07 - - $ */

%{

#include "lyutils.h"

#define YY_USER_ACTION  { lexer::advance(); }

int yylval_token (int symbol) {
   yylval = new astree (symbol, lexer::lloc, yytext);
   lexer::dumplexeme(yyout, yylval);
   return symbol;
}

%}

%option 8bit
%option debug
%option nodefault
%option nounput
%option noyywrap
%option verbose
%option warn

TOK_EQ          "=="
TOK_NE          "!="
TOK_LT          "<"
TOK_LE          "<="
TOK_GT          ">"
TOK_GE          ">="
TOK_ARRAY       "[]"

TOK_VOID        "void"
TOK_CHAR        "char"
TOK_INT         "int"
TOK_STRING      "string"
TOK_IF          "if"
TOK_ELSE        "else"
TOK_WHILE       "while"
TOK_RETURN      "return"
TOK_STRUCT      "struct"
TOK_NULL        "null"
TOK_NEW         "new"

TOK_IDENT       [_a-zA-Z][_a-zA-Z0-9]*
TOK_BAD_IDENT   [0-9]+[_a-zA-Z]+
TOK_INTCON      [0-9]+
TOK_CHARCON     ('([^\\'\n]|\\[\\'"0nt])')
TOK_STRINGCON   (\"([^\\"\n]|\\[\\'"0nt])*\")

%%

"#".*           { fprintf(yyout, "%s\n", yytext);
                  lexer::include(); }
[ \t]+          { }
\n              { lexer::newline(); }

"+"             { return yylval_token ('+'); }
"-"             { return yylval_token ('-'); }
"*"             { return yylval_token ('*'); }
"/"             { return yylval_token ('/'); }
"%"             { return yylval_token ('%'); }
"!="            { return yylval_token (TOK_NE); }
"!"             { return yylval_token ('!'); }
"=="            { return yylval_token (TOK_EQ); }
"="             { return yylval_token ('='); }
"<="            { return yylval_token (TOK_LE); }
"<"             { return yylval_token (TOK_LT); }
">="            { return yylval_token (TOK_GE); }
">"             { return yylval_token (TOK_GT); }
","             { return yylval_token (','); }
"."             { return yylval_token ('.'); }
";"             { return yylval_token (';'); }
"("             { return yylval_token ('('); }
")"             { return yylval_token (')'); }
"[]"            { return yylval_token (TOK_ARRAY); }
"["             { return yylval_token ('['); }
"]"             { return yylval_token (']'); }
"{"             { return yylval_token ('{'); }
"}"             { return yylval_token ('}'); }

{TOK_VOID}      { return yylval_token (TOK_VOID); }
{TOK_CHAR}      { return yylval_token (TOK_CHAR); }
{TOK_INT}       { return yylval_token (TOK_INT); }
{TOK_STRING}    { return yylval_token (TOK_STRING); }
{TOK_IF}        { return yylval_token (TOK_IF); }
{TOK_ELSE}      { return yylval_token (TOK_ELSE); }
{TOK_WHILE}     { return yylval_token (TOK_WHILE); }
{TOK_RETURN}    { return yylval_token (TOK_RETURN); }
{TOK_STRUCT}    { return yylval_token (TOK_STRUCT); }
{TOK_NULL}      { return yylval_token (TOK_NULL); }
{TOK_NEW}       { return yylval_token (TOK_NEW); }

{TOK_IDENT}     { return yylval_token (TOK_IDENT); }
{TOK_BAD_IDENT} { lexer::badtoken(yytext); }
{TOK_INTCON}    { return yylval_token (TOK_INTCON); }
{TOK_CHARCON}   { return yylval_token (TOK_CHARCON); }
{TOK_STRINGCON} { return yylval_token (TOK_STRINGCON); }

.               { lexer::badchar(*yytext); }

%%
