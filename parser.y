%{
#include "parser.tab.h"
%}

%token IF ELSE WHILE
%token MUDAR PISCAR LER ESPERAR
%token VERDE AMARELO VERMELHO
%token HORARIO DURACAO FLUXO
%token LPAREN RPAREN LBRACE RBRACE SEMI COMMA ASSIGN
%token PLUS MINUS STAR SLASH
%token EQ NEQ GT LT GTE LTE
%token AND OR NOT
%token ARROW
%token NUMBER IDENT
%token INVALID

%left OR
%left AND
%nonassoc EQ NEQ
%nonassoc LT GT LTE GTE
%left PLUS MINUS
%left STAR SLASH
%right NOT
%right UMINUS

%start program

%%

program:
      /* vazio */
    | program statement
    ;

statement:
      assignment SEMI
    | if_stmt
    | while_stmt
    | command SEMI
    ;

assignment:
      IDENT ASSIGN expression
    ;

if_stmt:
      IF LPAREN expression RPAREN block
    | IF LPAREN expression RPAREN block ELSE block
    ;

while_stmt:
      WHILE LPAREN expression RPAREN block
    ;

block:
      LBRACE statements_opt RBRACE
    ;

statements_opt:
      /* vazio */
    | program
    ;

command:
      MUDAR LPAREN color RPAREN
    | PISCAR LPAREN color COMMA expression RPAREN
    | LER LPAREN sensor RPAREN ARROW IDENT
    | ESPERAR LPAREN expression RPAREN
    ;

color:
      VERDE
    | AMARELO
    | VERMELHO
    ;

sensor:
      HORARIO
    | DURACAO
    | FLUXO
    ;

expression:
      LPAREN expression RPAREN
    | expression PLUS expression
    | expression MINUS expression
    | expression STAR expression
    | expression SLASH expression
    | expression LT expression
    | expression GT expression
    | expression LTE expression
    | expression GTE expression
    | expression EQ expression
    | expression NEQ expression
    | expression AND expression
    | expression OR expression
    | NOT expression
    | MINUS expression %prec UMINUS
    | NUMBER
    | IDENT
    ;

%%
