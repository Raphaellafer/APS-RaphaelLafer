%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "vm.h"

int yylex(void);
void yyerror(const char *s);
extern int yylineno;
extern FILE *yyin;

enum Cor { VERDE_C, AMARELO_C, VERMELHO_C };
enum Cor cor_atual;

/* VM global para gerar assembly */
VM_State *vm = NULL; /* Tornado não-static para acesso externo */
static int gerar_asm = 1; /* Flag para indicar se deve gerar assembly (sempre ativo) */
int executar_durante_compilacao = 0; /* Flag para indicar se deve executar durante compilação (tornado não-static para acesso externo) */

/* Pilha de labels para condicionais aninhadas */
#define MAX_LABEL_STACK 32
static char *pending_else_label_stack[MAX_LABEL_STACK];
static int pending_else_label_top = -1;
static char *pending_end_label_stack[MAX_LABEL_STACK];
static int pending_end_label_top = -1;

static void push_else_label(char *label) {
    if (pending_else_label_top < MAX_LABEL_STACK - 1) {
        pending_else_label_stack[++pending_else_label_top] = label;
    }
}

static char *pop_else_label(void) {
    if (pending_else_label_top >= 0) {
        return pending_else_label_stack[pending_else_label_top--];
    }
    return NULL;
}

static void push_end_label(char *label) {
    if (pending_end_label_top < MAX_LABEL_STACK - 1) {
        pending_end_label_stack[++pending_end_label_top] = label;
    }
}

static char *pop_end_label(void) {
    if (pending_end_label_top >= 0) {
        return pending_end_label_stack[pending_end_label_top--];
    }
    return NULL;
}

/* Função para obter ou criar a VM */
static VM_State* get_vm(void) {
    if (!vm) {
        vm = vm_create();
    }
    return vm;
}

/* Função para gerar label único */
static int label_counter = 0;
static char* new_label(void) {
    char *label = malloc(16);
    sprintf(label, "L%d", label_counter++);
    return label;
}

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
static int last_cond_value = 0; /* Valor booleano da última condição */

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

/* Simulação de tempo: horário simulado que avança */
static int horario_simulado = -1; /* -1 = não inicializado */
static int minutos_simulados = 0; /* minutos acumulados desde o início */

/* Estrutura para armazenar informações sobre while loops para reexecução */
typedef struct {
    char *var_name;      /* nome da variável na condição (se houver) */
    int op;              /* operador (0=LT, 1=GT, 2=LTE, 3=GTE, 4=EQ, 5=NEQ) */
    int value;           /* valor de comparação */
} WhileCondition;

static WhileCondition while_conditions[100];
static int while_condition_count = 0;

static int ler_sensor(const char *s) {
    int valor = 0;
    if (strcmp(s, "horario") == 0) {
        /* Se não foi inicializado, usa o horário real do sistema */
        if (horario_simulado == -1) {
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            horario_simulado = tm->tm_hour;
            minutos_simulados = tm->tm_min;
            if (executar_durante_compilacao) {
                printf("  -> Horário inicial: %d:%02d\n", horario_simulado, minutos_simulados);
            }
        } else {
            /* Calcula o horário baseado nos minutos simulados */
            int horas_totais = horario_simulado * 60 + minutos_simulados;
            int hora_atual = (horas_totais / 60) % 24;
            int minuto_atual = horas_totais % 60;
            if (executar_durante_compilacao) {
                printf("  -> Horário atual: %d:%02d\n", hora_atual, minuto_atual);
            }
            valor = hora_atual;
        }
        valor = horario_simulado;
    } else if (strcmp(s, "duracao") == 0) {
        valor = rand() % 10 + 1; /* Simula duração entre 1 e 10 segundos */
        if (executar_durante_compilacao) {
            printf("  -> Duração detectada: %d segundos\n", valor);
        }
    } else if (strcmp(s, "fluxo") == 0) {
        valor = rand() % 50 + 1;
        if (executar_durante_compilacao) {
            printf("  -> Fluxo detectado: %d veiculos\n", valor);
        }
    }
    return valor;
}

static void executar_comando(const char *cmd, const char *param, int valor) {
    if (!executar_durante_compilacao) {
        return; /* Não executa durante compilação */
    }
    
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
        /* Simula a passagem do tempo: incrementa minutos baseado nos segundos */
        /* Para simulação rápida, vamos incrementar 1 minuto a cada segundo esperado */
        /* Ou seja, se esperar 30 segundos, avança 30 minutos no horário simulado */
        if (horario_simulado != -1) {
            minutos_simulados += valor; /* valor está em segundos, mas vamos tratar como minutos */
            /* Se passar de 60 minutos, incrementa a hora */
            if (minutos_simulados >= 60) {
                horario_simulado = (horario_simulado + minutos_simulados / 60) % 24;
                minutos_simulados = minutos_simulados % 60;
            }
        }
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
%type <num> while_stmt while_condition_start while_condition_end while_block_start while_block while_block_end
%type <str> condition_label

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
    { 
        exec_top = 1; 
        exec_stack[0] = 1; 
        label_counter = 0;
        if (vm) {
            vm_destroy(vm);
            vm = NULL;
        }
        get_vm(); /* Inicializa a VM */
    }
    statements { 
        /* Adiciona HALT no final */
        vm_add_instruction(vm, VM_HALT, 0, 0, NULL);
        YYACCEPT; 
    }
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
        if (current_exec()) { 
            set_var($1, $3);
            printf("  -> Variável '%s' = %d\n", $1, $3);
        }
        /* Sempre gera assembly, independente de current_exec() */
        /* Gera assembly: expression já deixou resultado em R0 */
        int addr = vm_alloc_var(get_vm(), $1);
        vm_add_instruction(get_vm(), VM_STORE, addr, 0, NULL);
        free($1);
    }
    ;

if_stmt:
    IF LPAREN condition_set RPAREN then_start block then_end
    {
        /* then_start ($5) gerou JZ com false_label, então geramos o label aqui */
        char *false_label = (char*)$5; /* $5 é o then_start que retornou o label */
        if (false_label) {
            vm_add_instruction(get_vm(), VM_NOP, 0, 0, false_label);
            free(false_label);
        }
        push_exec(current_exec() && last_cond);
        pop_exec();
    }
    | IF LPAREN condition_set RPAREN then_start block then_end ELSE else_start block else_end
    {
        /* then_start ($5) gerou JZ que aponta para o label do else */
        /* else_start ($8) já gerou o label do else */
        /* then_end ($7) já gerou JMP para pular o else */
        /* Precisamos gerar o label do fim (que foi usado pelo JMP do then_end) */
        char *end_label = pop_end_label();
        if (end_label) {
            vm_add_instruction(get_vm(), VM_NOP, 0, 0, end_label);
            free(end_label);
        }
        push_exec(current_exec() && last_cond);
        pop_exec();
        push_exec(current_exec() && !last_cond);
        pop_exec();
    }
    ;

condition_set:
    condition { $$ = $1; last_cond = last_cond_value; }
    ;

then_start:
    { 
        /* condition deixou resultado na stack (via logic_or) */
        /* Restaura resultado da stack para R0 antes do JZ */
        vm_add_instruction(get_vm(), VM_POP, 0, 0, NULL); /* Restaura resultado para R0 */
        /* Cria um novo label para o JZ */
        char *false_label = new_label();
        push_else_label(strdup(false_label)); /* Armazena na pilha para uso no else_start */
        vm_add_instruction(get_vm(), VM_JZ, 0, 0, false_label);
        $$ = (int)false_label; /* Retorna o label como ponteiro */
        push_exec(current_exec() && last_cond);
    }
    ;

then_end:
    { 
        pop_exec(); 
        /* Se há um else pendente, geramos JMP aqui para pular o else */
        /* Verificamos se há um label do else na pilha (indicando que há um else) */
        if (pending_else_label_top >= 0) {
            char *end_label = new_label();
            vm_add_instruction(get_vm(), VM_JMP, 0, 0, end_label);
            /* Armazena o label do fim em uma pilha separada */
            push_end_label(end_label);
        }
        $$ = 0; 
    }
    ;

else_start:
    { 
        /* Gera label para o início do else */
        /* Usa o label que foi gerado pelo then_start (da pilha) */
        char *else_label = pop_else_label();
        if (else_label) {
            vm_add_instruction(get_vm(), VM_NOP, 0, 0, else_label);
            free(else_label);
        } else {
            /* Fallback: cria novo label se pilha estiver vazia */
            char *new_else_label = new_label();
            vm_add_instruction(get_vm(), VM_NOP, 0, 0, new_else_label);
            free(new_else_label);
        }
        $$ = 0;
        push_exec(current_exec() && !last_cond); 
    }
    ;

else_end:
    { 
        pop_exec(); 
        $$ = 0; 
    }
    ;

while_stmt:
    WHILE LPAREN while_condition_start while_condition_end RPAREN while_block_start while_block while_block_end
    {
        /* Para assembly: código de loop já foi gerado com labels e jumps */
        $$ = 0;
    }
    ;

while_condition_start:
    {
        /* Para assembly: cria label de início do loop */
        char *loop_start_label = new_label();
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, loop_start_label);
        /* Armazena o label para uso no while_block_end */
        push_else_label(loop_start_label); /* Reutiliza a pilha de else para armazenar loop_start */
        $$ = 0;
    }
    ;

while_condition_end:
    condition
    {
        /* Para assembly: avalia condição e pula para fim se falsa */
        /* condition deixou resultado na stack */
        vm_add_instruction(get_vm(), VM_POP, 0, 0, NULL); /* Restaura resultado para R0 */
        char *loop_end_label = new_label();
        vm_add_instruction(get_vm(), VM_JZ, 0, 0, loop_end_label); /* Se falso, sai do loop */
        /* Armazena o label do fim para uso no while_block_end */
        push_end_label(loop_end_label);
        /* Para execução interpretada: armazena o valor da condição */
        last_cond_value = ($1 != 0) ? 1 : 0;
        push_exec(current_exec() && last_cond_value);
        $$ = $1;
    }
    ;

while_block_start:
    {
        /* Para execução interpretada: início do bloco */
        $$ = 0;
    }
    ;

while_block:
    LBRACE statements RBRACE
    {
        $$ = 0;
    }
    ;

while_block_end:
    {
        /* Para assembly: volta para o início do loop */
        char *loop_start_label = pop_else_label(); /* Recupera o label de início */
        if (loop_start_label) {
            vm_add_instruction(get_vm(), VM_JMP, 0, 0, loop_start_label);
            free(loop_start_label);
        }
        /* Gera label do fim do loop */
        char *loop_end_label = pop_end_label();
        if (loop_end_label) {
            vm_add_instruction(get_vm(), VM_NOP, 0, 0, loop_end_label);
            free(loop_end_label);
        }
        pop_exec();
        $$ = 0;
    }
    ;

block:
    LBRACE statements RBRACE
    ;

command:
    MUDAR LPAREN color RPAREN { 
        if (current_exec()) { 
            executar_comando("mudar", $3, 0);
        }
        /* Sempre gera assembly, independente de current_exec() */
        int cor = 0;
        if (strcmp($3, "verde") == 0) cor = 0;
        else if (strcmp($3, "amarelo") == 0) cor = 1;
        else cor = 2;
        vm_add_instruction(get_vm(), VM_MUDAR, cor, 0, NULL);
        free($3); 
    }
    | PISCAR LPAREN color COMMA expression RPAREN { 
        if (current_exec()) { 
            executar_comando("piscar", $3, $5);
            /* Gera assembly: expression já deixou resultado em R0 */
            int cor = 0;
            if (strcmp($3, "verde") == 0) cor = 0;
            else if (strcmp($3, "amarelo") == 0) cor = 1;
            else cor = 2;
            vm_add_instruction(get_vm(), VM_PISCAR, cor, $5, NULL);
        }
        free($3); 
    }
    | LER LPAREN sensor RPAREN ARROW IDENT { 
        if (current_exec()) { 
            int v = ler_sensor($3);
            set_var($6, v);
            /* Gera assembly */
            int sensor_id = 0;
            if (strcmp($3, "horario") == 0) sensor_id = SENSOR_HORARIO;
            else if (strcmp($3, "duracao") == 0) sensor_id = SENSOR_DURACAO;
            else if (strcmp($3, "fluxo") == 0) sensor_id = SENSOR_FLUXO;
            vm_add_instruction(get_vm(), VM_READ_SENSOR, sensor_id, 0, NULL);
            int addr = vm_alloc_var(get_vm(), $6);
            vm_add_instruction(get_vm(), VM_STORE, addr, 0, NULL);
        }
        free($3); 
        free($6); 
    }
    | ESPERAR LPAREN expression RPAREN { 
        if (current_exec()) { 
            executar_comando("esperar", "", $3);
        }
        /* Sempre gera assembly, independente de current_exec() */
        /* Gera assembly: ESPERAR usa o valor em R0 que já foi carregado pela expression */
        /* O valor já está em R0, então ESPERAR não precisa de argumento */
        vm_add_instruction(get_vm(), VM_ESPERAR, 0, 0, NULL);
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
    {
        /* logic_or deixou resultado na stack (do logic_and) */
        /* Não fazemos POP aqui, o resultado fica na stack */
        /* O then_start fará POP antes do JZ */
        /* Armazena o valor booleano da condição para uso em last_cond */
        last_cond_value = ($1 != 0) ? 1 : 0;
        /* Gera label para pular se falso */
        char *false_label = new_label();
        /* JZ será gerado pelo then_start, então apenas retornamos o label */
        $$ = (int)false_label;
    }
    ;

logic_or:
    logic_and { 
        $$ = $1;
        /* logic_and já deixou resultado na stack, não precisa fazer PUSH novamente */
    }
    | logic_or OR logic_and { 
        /* Para execução interpretada: calcula OR lógico corretamente */
        /* $1 e $3 são valores semânticos (0 ou 1) */
        /* $1 é o resultado do logic_or anterior, $3 é o resultado do logic_and */
        int val1 = ($1 != 0) ? 1 : 0;
        int val3 = ($3 != 0) ? 1 : 0;
        $$ = (val1 || val3) ? 1 : 0;
        /* Short-circuit: se $1 é verdadeiro, não avalia $3 */
        /* Stack: [..., $1] (salvo pelo logic_or anterior) */
        /* R0: $3 (resultado do logic_and atual, que também fez PUSH) */
        /* Stack agora: [..., $1, $3] onde $3 está no topo */
        /* Precisamos: se $1 é 1, resultado é 1; senão resultado é $3 */
        /* Salva $3 antes de verificar $1 */
        vm_add_instruction(get_vm(), VM_POP, 0, 0, NULL); /* Remove $3 do topo, deixa em R0 */
        int temp_addr = vm_alloc_var(get_vm(), "__temp_or");
        vm_add_instruction(get_vm(), VM_STORE, temp_addr, 0, NULL); /* Salva $3 em memória */
        /* Restaura $1 da stack para R0 */
        vm_add_instruction(get_vm(), VM_POP, 0, 0, NULL); /* Remove $1, deixa em R0 */
        char *true_label = new_label();
        char *end_label = new_label();
        vm_add_instruction(get_vm(), VM_JNZ, 0, 0, true_label); /* Se $1 é 1, resultado é 1 */
        /* $1 é 0, então resultado é $3 - restaura $3 da memória */
        vm_add_instruction(get_vm(), VM_LOADM, temp_addr, 0, NULL); /* Restaura $3 */
        vm_add_instruction(get_vm(), VM_JMP, 0, 0, end_label);
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, true_label);
        vm_add_instruction(get_vm(), VM_LOAD, 1, 0, NULL); /* Resultado é 1 */
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, end_label);
        /* Resultado final está em R0, salva na stack para possível OR seguinte */
        vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL);
        free(true_label);
        free(end_label);
    }
    ;

logic_and:
    rel_condition { 
        $$ = $1;
        /* Resultado está em R0, salva na stack para possível uso em AND seguinte */
        vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL);
    }
    | logic_and AND rel_condition { 
        /* Para execução interpretada: calcula AND lógico corretamente */
        /* $1 e $3 são valores semânticos (0 ou 1) */
        int val1 = ($1 != 0) ? 1 : 0;
        int val3 = ($3 != 0) ? 1 : 0;
        $$ = (val1 && val3) ? 1 : 0;
        /* Short-circuit AND: se $1 é falso, resultado é falso, senão é $3 */
        /* Stack: [..., $1] (salvo pela regra anterior) */
        /* R0: $3 (resultado do rel_condition, que também fez PUSH) */
        /* Stack agora: [..., $1, $3] onde $3 está no topo */
        /* Precisamos: se $1 é 0, resultado é 0; senão resultado é $3 */
        char *false_label = new_label();
        char *end_label = new_label();
        /* Salva $3 em memória temporária antes de verificar $1 */
        vm_add_instruction(get_vm(), VM_POP, 0, 0, NULL); /* Remove $3 do topo, deixa em R0 */
        int temp_addr = vm_alloc_var(get_vm(), "__temp_and");
        vm_add_instruction(get_vm(), VM_STORE, temp_addr, 0, NULL); /* Salva $3 em memória */
        /* Restaura $1 da stack para R0 */
        vm_add_instruction(get_vm(), VM_POP, 0, 0, NULL); /* Remove $1, deixa em R0 */
        vm_add_instruction(get_vm(), VM_JZ, 0, 0, false_label); /* Se $1 é 0, pula para false */
        /* $1 não é 0, então resultado é $3 - restaura $3 da memória */
        vm_add_instruction(get_vm(), VM_LOADM, temp_addr, 0, NULL); /* Restaura $3 */
        vm_add_instruction(get_vm(), VM_JMP, 0, 0, end_label);
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, false_label);
        /* $1 é 0, então resultado é 0 */
        vm_add_instruction(get_vm(), VM_LOAD, 0, 0, NULL); /* Resultado é 0 */
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, end_label);
        /* Resultado final está em R0, salva na stack para possível AND seguinte */
        vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL);
        free(false_label);
        free(end_label);
    }
    ;

rel_condition:
    expression { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); } LT expression { 
        /* Para execução interpretada: $1 é expression1, $4 é expression2 */
        int expr1 = $1;
        int expr2 = $4;
        $$ = (expr1 < expr2) ? 1 : 0;
        /* expression1 foi salvo na stack pela mid-rule action */
        /* expression2 está em R0 */
        vm_add_instruction(get_vm(), VM_CMP, 0, 0, NULL);
        /* CMP deixa (stack - R0) em R0, ou seja (expression1 - expression2) */
        /* Se R0 < 0, então expression1 < expression2 */
        char *true_label = new_label();
        char *end_label = new_label();
        vm_add_instruction(get_vm(), VM_JLT, 0, 0, true_label);
        vm_add_instruction(get_vm(), VM_LOAD, 0, 0, NULL); /* Falso: carrega 0 */
        vm_add_instruction(get_vm(), VM_JMP, 0, 0, end_label);
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, true_label);
        vm_add_instruction(get_vm(), VM_LOAD, 1, 0, NULL); /* Verdadeiro: carrega 1 */
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, end_label);
        free(true_label);
        free(end_label);
    }
    | expression { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); } GT expression { 
        /* Para execução interpretada: $1 é expression1, $4 é expression2 */
        int expr1 = $1;
        int expr2 = $4;
        $$ = (expr1 > expr2) ? 1 : 0;
        vm_add_instruction(get_vm(), VM_CMP, 0, 0, NULL);
        char *true_label = new_label();
        char *end_label = new_label();
        vm_add_instruction(get_vm(), VM_JGT, 0, 0, true_label);
        vm_add_instruction(get_vm(), VM_LOAD, 0, 0, NULL);
        vm_add_instruction(get_vm(), VM_JMP, 0, 0, end_label);
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, true_label);
        vm_add_instruction(get_vm(), VM_LOAD, 1, 0, NULL);
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, end_label);
        free(true_label);
        free(end_label);
    }
    | expression { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); } LTE expression { 
        /* Para execução interpretada: $1 é expression1, $4 é expression2 */
        int expr1 = $1;
        int expr2 = $4;
        $$ = (expr1 <= expr2) ? 1 : 0;
        vm_add_instruction(get_vm(), VM_CMP, 0, 0, NULL);
        char *true_label = new_label();
        char *end_label = new_label();
        vm_add_instruction(get_vm(), VM_JLE, 0, 0, true_label);
        vm_add_instruction(get_vm(), VM_LOAD, 0, 0, NULL);
        vm_add_instruction(get_vm(), VM_JMP, 0, 0, end_label);
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, true_label);
        vm_add_instruction(get_vm(), VM_LOAD, 1, 0, NULL);
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, end_label);
        free(true_label);
        free(end_label);
    }
    | expression { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); } GTE expression { 
        /* Para execução interpretada: $1 é expression1, $4 é expression2 */
        int expr1 = $1;
        int expr2 = $4;
        $$ = (expr1 >= expr2) ? 1 : 0;
        vm_add_instruction(get_vm(), VM_CMP, 0, 0, NULL);
        char *true_label = new_label();
        char *end_label = new_label();
        vm_add_instruction(get_vm(), VM_JGE, 0, 0, true_label);
        vm_add_instruction(get_vm(), VM_LOAD, 0, 0, NULL);
        vm_add_instruction(get_vm(), VM_JMP, 0, 0, end_label);
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, true_label);
        vm_add_instruction(get_vm(), VM_LOAD, 1, 0, NULL);
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, end_label);
        free(true_label);
        free(end_label);
    }
    | expression { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); } EQ expression { 
        $$ = ($1 == $4);
        vm_add_instruction(get_vm(), VM_EQ, 0, 0, NULL);
        /* EQ já deixa 1 ou 0 em R0 */
    }
    | expression { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); } NEQ expression { 
        $$ = ($1 != $4);
        vm_add_instruction(get_vm(), VM_NE, 0, 0, NULL);
        /* NE já deixa 1 ou 0 em R0 */
    }
    | NOT rel_condition { 
        $$ = !$2;
        /* Inverte o resultado em R0: se 1 vira 0, se 0 vira 1 */
        vm_add_instruction(get_vm(), VM_LOAD, 1, 0, NULL);
        vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL);
        vm_add_instruction(get_vm(), VM_SUB, 0, 0, NULL);
    }
    | LPAREN condition RPAREN { $$ = $2; }
    ;

expression:
    term { $$ = $1; }
    | expression { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); } PLUS term { 
        /* Para execução interpretada: $1 é expression, $4 é term */
        $$ = $1 + $4;
        /* Gera assembly: soma */
        /* expression foi salvo na stack pela mid-rule action */
        /* term está em R0 */
        vm_add_instruction(get_vm(), VM_ADD, 0, 0, NULL);
    }
    | expression { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); } MINUS term { 
        /* Para execução interpretada: $1 é expression, $4 é term */
        $$ = $1 - $4;
        /* Gera assembly: subtrai */
        /* expression foi salvo na stack pela mid-rule action */
        /* term está em R0 */
        vm_add_instruction(get_vm(), VM_SUB, 0, 0, NULL);
    }
    | expression { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); } STAR term { 
        /* Para execução interpretada: $1 é expression, $4 é term */
        $$ = $1 * $4;
        /* Gera assembly: multiplica */
        /* expression foi salvo na stack pela mid-rule action */
        /* term está em R0 */
        vm_add_instruction(get_vm(), VM_MUL, 0, 0, NULL);
    }
    | expression { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); } SLASH term { 
        /* Para execução interpretada: $1 é expression, $4 é term */
        $$ = ($4 != 0) ? ($1 / $4) : 0;
        /* Gera assembly: divide */
        /* expression foi salvo na stack pela mid-rule action */
        /* term está em R0 */
        vm_add_instruction(get_vm(), VM_DIV, 0, 0, NULL);
    }
    ;

term:
    NUMBER { 
        $$ = $1;
        /* Gera assembly: carrega número em R0 */
        vm_add_instruction(get_vm(), VM_LOAD, $1, 0, NULL);
    }
    | IDENT { 
        $$ = get_var($1);
        /* Gera assembly: carrega variável da memória */
        int addr = vm_alloc_var(get_vm(), $1);
        vm_add_instruction(get_vm(), VM_LOADM, addr, 0, NULL);
        free($1); 
    }
    | MINUS term %prec UMINUS { 
        $$ = -$2;
        /* Gera assembly: nega o valor em R0 */
        vm_add_instruction(get_vm(), VM_LOAD, 0, 0, NULL);
        vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL);
        vm_add_instruction(get_vm(), VM_SUB, 0, 0, NULL);
    }
    | LPAREN expression RPAREN { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "ERRO SINTÁTICO (linha %d): %s\n", yylineno, s);
}