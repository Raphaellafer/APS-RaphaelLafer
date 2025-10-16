%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int yylex(void);
void yyerror(const char *s);
extern int yylineno;
extern FILE *yyin;

enum Cor { VERDE_C, AMARELO_C, VERMELHO_C };
enum Cor cor_atual;

typedef struct { 
    char *name; 
    int val; 
} Var;

static Var vars[256];
static int nvars = 0;

static int get_var(const char *name) {
    for (int i = 0; i < nvars; i++) 
        if (strcmp(vars[i].name, name) == 0) 
            return vars[i].val;
    return 0;
}

static void set_var(const char *name, int v) {
    for (int i = 0; i < nvars; i++) 
        if (strcmp(vars[i].name, name) == 0) { 
            vars[i].val = v; 
            return; 
        }
    if (nvars < 256) { 
        vars[nvars].name = strdup(name); 
        vars[nvars].val = v; 
        nvars++; 
    }
}

static int ler_sensor(const char *s) {
    if (strcmp(s, "horario") == 0) {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        int hora = tm->tm_hour;
        if (hora >= 6 && hora < 20) {
            printf("  -> Horário atual: entre 6 e 20 hrs (%d horas)\n", hora);
        } else {
            printf("  -> Horário atual: após 20 hrs (%d horas)\n", hora);
        }
        return hora;
    }
    if (strcmp(s, "duracao") == 0) return 0;
    if (strcmp(s, "fluxo") == 0) return rand() % 50 + 1;
    return 0;
}

static void executar_comando(const char *cmd, const char *param, int valor) {
    if (strcmp(cmd, "mudar") == 0) {
        printf("  -> Mudando semáforo para %s\n", param);
        if (strcmp(param, "verde") == 0) cor_atual = VERDE_C;
        else if (strcmp(param, "amarelo") == 0) cor_atual = AMARELO_C;
        else cor_atual = VERMELHO_C;
    }
    else if (strcmp(cmd, "esperar") == 0) {
        printf("  -> Esperando %d segundos...\n", valor);
    }
}
%}

%union {
    int   num;
    char* str;
}

%token IF ELSE WHILE
%token MUDAR PISCAR LER ESPERAR
%token <str> VERDE AMARELO VERMELHO
%token <str> HORARIO DURACAO FLUXO
%token LPAREN RPAREN LBRACE RBRACE COMMA ASSIGN SEMI
%token PLUS MINUS STAR SLASH
%token EQ NEQ GT LT GTE LTE
%token AND OR NOT
%token ARROW
%token <num> NUMBER
%token <str> IDENT
%token FIM
%token NEWLINE
%token INVALID

%type <str> color sensor
%type <num> expression condition

%left OR
%left AND
%nonassoc EQ NEQ
%nonassoc LT GT LTE GTE
%left PLUS MINUS
%left STAR SLASH
%right UMINUS

%start program

%%

program:
    statements { YYACCEPT; }
    ;

statements:
    statement
    | statements statement
    ;

statement:
    assignment SEMI
    | command SEMI
    | if_stmt
    | while_stmt
    | block
    | SEMI
    ;

assignment:
    IDENT ASSIGN expression { 
        set_var($1, $3); 
        free($1); 
    }
    ;

if_stmt:
    IF LPAREN condition RPAREN block
    | IF LPAREN condition RPAREN block ELSE block
    ;

while_stmt:
    WHILE LPAREN condition RPAREN block
    ;

block:
    LBRACE statements RBRACE
    ;

command:
    MUDAR LPAREN color RPAREN { 
        executar_comando("mudar", $3, 0); 
        free($3); 
    }
    | LER LPAREN sensor RPAREN ARROW IDENT { 
        int v = ler_sensor($3); 
        set_var($6, v);
        free($3); 
        free($6); 
    }
    | ESPERAR LPAREN expression RPAREN { 
        executar_comando("esperar", "", $3); 
    }
    ;

color:
    VERDE { $$ = strdup("verde"); }
    | AMARELO { $$ = strdup("amarelo"); }
    | VERMELHO { $$ = strdup("vermelho"); }
    ;

sensor:
    HORARIO { $$ = strdup("horario"); }
    | DURACAO { $$ = strdup("duracao"); }
    | FLUXO { $$ = strdup("fluxo"); }
    ;

condition:
    expression LT expression { $$ = ($1 < $3); }
    | expression GT expression { $$ = ($1 > $3); }
    | expression LTE expression { $$ = ($1 <= $3); }
    | expression GTE expression { $$ = ($1 >= $3); }
    | expression EQ expression { $$ = ($1 == $3); }
    | expression NEQ expression { $$ = ($1 != $3); }
    | condition AND condition { $$ = ($1 && $3); }
    | condition OR condition { $$ = ($1 || $3); }
    | NOT condition { $$ = (!$2); }
    | LPAREN condition RPAREN { $$ = $2; }
    ;

expression:
    NUMBER { $$ = $1; }
    | IDENT { $$ = get_var($1); free($1); }
    | expression PLUS expression { $$ = $1 + $3; }
    | expression MINUS expression { $$ = $1 - $3; }
    | expression STAR expression { $$ = $1 * $3; }
    | expression SLASH expression { $$ = $3 != 0 ? $1 / $3 : 0; }
    | MINUS expression %prec UMINUS { $$ = -$2; }
    | LPAREN expression RPAREN { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
}