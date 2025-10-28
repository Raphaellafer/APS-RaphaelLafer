%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

int yylex(void);
void yyerror(const char *s);
extern int yylineno;
extern FILE *yyin;

enum Cor { VERDE_C, AMARELO_C, VERMELHO_C };
enum Cor cor_atual;

/* Sem emissor de assembly: comportamento interpretado conforme README */

/* Execução condicional: pilha de flags */
static int exec_stack[256];
static int exec_top = 0;
static inline int current_exec(void) {
    return exec_top > 0 ? exec_stack[exec_top - 1] : 1;
}
static inline void push_exec(int flag) {
    if (exec_top < 256) exec_stack[exec_top++] = flag ? 1 : 0;
}
static inline void pop_exec(void) {
    if (exec_top > 0) exec_top--;
}
static int last_cond = 0;

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
        printf("  -> Horário atual: (%d horas)\n", hora);
        return hora;
    }
    if (strcmp(s, "duracao") == 0) return 0;
    if (strcmp(s, "fluxo") == 0) {
        int v = rand() % 50 + 1;
        printf("  -> Fluxo detectado: %d veiculos\n", v);
        return v;
    }
    return 0;
}

static void executar_comando(const char *cmd, const char *param, int valor) {
    if (strcmp(cmd, "mudar") == 0) {
        printf("  -> Mudando semáforo para %s\n", param);
        if (strcmp(param, "verde") == 0) cor_atual = VERDE_C;
        else if (strcmp(param, "amarelo") == 0) cor_atual = AMARELO_C;
        else cor_atual = VERMELHO_C;
    }
    else if (strcmp(cmd, "piscar") == 0) {
        printf("  -> Piscando %s por %d vezes\n", param, valor);
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
%type <num> expression term logic_or logic_and rel_condition condition
%type <num> condition_set
%type <num> then_start then_end else_start else_end

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
    { exec_top = 1; exec_stack[0] = 1; }
    statements { YYACCEPT; }
    | error { yyclearin; yyerrok; YYABORT; }
    ;

statements:
    statement
    | statements statement
    ;

statement:
    assignment terminator
    | command terminator
    | if_stmt
    | while_stmt
    | block
    | terminator
    ;

terminator:
    SEMI
    | NEWLINE
    ;

assignment:
    IDENT ASSIGN expression { 
        if (current_exec()) { set_var($1, $3); }
        free($1);
    }
    ;

if_stmt:
    IF LPAREN condition_set RPAREN then_start block then_end
    | IF LPAREN condition_set RPAREN then_start block then_end ELSE else_start block else_end
    ;

condition_set:
    condition { $$ = $1; last_cond = $1; }
    ;

then_start:
    { push_exec(current_exec() && last_cond); $$ = 0; }
    ;

then_end:
    { pop_exec(); $$ = 0; }
    ;

else_start:
    { push_exec(current_exec() && !last_cond); $$ = 0; }
    ;

else_end:
    { pop_exec(); $$ = 0; }
    ;

while_stmt:
    WHILE LPAREN condition RPAREN block
    ;

block:
    LBRACE statements RBRACE
    ;

command:
    MUDAR LPAREN color RPAREN { 
        if (current_exec()) { executar_comando("mudar", $3, 0); }
        free($3); 
    }
    | PISCAR LPAREN color COMMA expression RPAREN { 
        if (current_exec()) { executar_comando("piscar", $3, $5); }
        free($3); 
    }
    | LER LPAREN sensor RPAREN ARROW IDENT { 
        if (current_exec()) { int v = ler_sensor($3); set_var($6, v); }
        free($3); 
        free($6); 
    }
    | ESPERAR LPAREN expression RPAREN { 
        if (current_exec()) { executar_comando("esperar", "", $3); }
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
    logic_or
    ;

logic_or:
    logic_and
    | logic_or OR logic_and
    ;

logic_and:
    rel_condition
    | logic_and AND rel_condition
    ;

rel_condition:
    expression LT expression { $$ = ($1 < $3); }
    | expression GT expression { $$ = ($1 > $3); }
    | expression LTE expression { $$ = ($1 <= $3); }
    | expression GTE expression { $$ = ($1 >= $3); }
    | expression EQ expression { $$ = ($1 == $3); }
    | expression NEQ expression { $$ = ($1 != $3); }
    | NOT rel_condition { $$ = !$2; }
    | LPAREN condition RPAREN { $$ = $2; }
    ;

expression:
    term
    | expression PLUS term { $$ = $1 + $3; }
    | expression MINUS term { $$ = $1 - $3; }
    | expression STAR term { $$ = $1 * $3; }
    | expression SLASH term { $$ = $3 != 0 ? $1 / $3 : 0; }
    ;

term:
    NUMBER { $$ = $1; }
    | IDENT { $$ = get_var($1); free($1); }
    | MINUS term %prec UMINUS { $$ = -$2; }
    | LPAREN expression RPAREN { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "ERRO SINTÁTICO (linha %d): %s\n", yylineno, s);
}