%option noyywrap
%option yylineno

%{
#include "parser.tab.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

extern FILE *yyin;

static int is_fim(const char *txt) {
    char lower[32] = {0};
    int i = 0;
    for (; txt[i] && i < (int)sizeof(lower)-1; i++) {
        lower[i] = (char)tolower((unsigned char)txt[i]);
    }
    return strcmp(lower, "fim") == 0;
}
%}

DIGITO      [0-9]
LETRA       [A-Za-z]
ID          {LETRA}({LETRA}|{DIGITO}|_)*

%%

"if"            return IF;
"else"          return ELSE;
"while"         return WHILE;

"mudar"         return MUDAR;
"piscar"        return PISCAR;
"ler"           return LER;
"esperar"       return ESPERAR;

"verde"         { yylval.str = strdup("verde"); return VERDE; }
"amarelo"       { yylval.str = strdup("amarelo"); return AMARELO; }
"vermelho"      { yylval.str = strdup("vermelho"); return VERMELHO; }

"horario"       { yylval.str = strdup("horario"); return HORARIO; }
"duracao"       { yylval.str = strdup("duracao"); return DURACAO; }
"fluxo"         { yylval.str = strdup("fluxo"); return FLUXO; }

"=="            return EQ;
"!="            return NEQ;
">="            return GTE;
"<="            return LTE;
"&&"            return AND;
"||"            return OR;
"->"            return ARROW;

">"             return GT;
"<"             return LT;
"="             return ASSIGN;
"!"             return NOT;
"+"             return PLUS;
"-"             return MINUS;
"*"             return STAR;
"/"             return SLASH;
","             return COMMA;
"("             return LPAREN;
")"             return RPAREN;
"{"             return LBRACE;
"}"             return RBRACE;
";"             return SEMI;

[ \t]+          /* ignore espaços e tabs */
\n              { return NEWLINE; }

"//"[^\n]*      /* ignore comentários */

{DIGITO}+       { yylval.num = atoi(yytext); return NUMBER; }
{ID}            {
    if (is_fim(yytext)) return FIM;
    yylval.str = strdup(yytext);
    return IDENT;
}

.               { 
    fprintf(stderr, "ERRO LÉXICO (linha %d): Caractere inválido '%s'\n", yylineno, yytext); 
    return INVALID; 
}

%%