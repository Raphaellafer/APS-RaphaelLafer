%option noyywrap
%option yylineno

%{
#include "parser.tab.h"
#include <stdlib.h>
%}

DIGITO      [0-9]
LETRA       [A-Za-z]
ID          {LETRA}({LETRA}|{DIGITO}|_)*

%%

if              { return IF; }
else            { return ELSE; }
while           { return WHILE; }

mudar           { return MUDAR; }
piscar          { return PISCAR; }
ler             { return LER; }
esperar         { return ESPERAR; }

verde           { return VERDE; }
amarelo         { return AMARELO; }
vermelho        { return VERMELHO; }

horario         { return HORARIO; }
duracao         { return DURACAO; }
fluxo           { return FLUXO; }

"=="            { return EQ; }
"!="            { return NEQ; }
">="            { return GTE; }
"<="            { return LTE; }
"&&"            { return AND; }
"||"            { return OR; }
"->"            { return ARROW; }

">"             { return GT; }
"<"             { return LT; }
"="             { return ASSIGN; }
"!"             { return NOT; }
"+"             { return PLUS; }
"-"             { return MINUS; }
"*"             { return STAR; }
"/"             { return SLASH; }
";"             { return SEMI; }
","             { return COMMA; }
"("             { return LPAREN; }
")"             { return RPAREN; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }

{DIGITO}+       { yylval = atoi(yytext); return NUMBER; }
{ID}            { return IDENT; }

"//".*          ;

[ \t\r\n]+      ;

.               { return INVALID; }

%%
