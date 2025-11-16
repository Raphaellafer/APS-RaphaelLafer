/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "parser.y"

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

#line 266 "parser.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_IF = 3,                         /* IF  */
  YYSYMBOL_ELSE = 4,                       /* ELSE  */
  YYSYMBOL_WHILE = 5,                      /* WHILE  */
  YYSYMBOL_MUDAR = 6,                      /* MUDAR  */
  YYSYMBOL_PISCAR = 7,                     /* PISCAR  */
  YYSYMBOL_LER = 8,                        /* LER  */
  YYSYMBOL_ESPERAR = 9,                    /* ESPERAR  */
  YYSYMBOL_VERDE = 10,                     /* VERDE  */
  YYSYMBOL_AMARELO = 11,                   /* AMARELO  */
  YYSYMBOL_VERMELHO = 12,                  /* VERMELHO  */
  YYSYMBOL_HORARIO = 13,                   /* HORARIO  */
  YYSYMBOL_DURACAO = 14,                   /* DURACAO  */
  YYSYMBOL_FLUXO = 15,                     /* FLUXO  */
  YYSYMBOL_LPAREN = 16,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 17,                    /* RPAREN  */
  YYSYMBOL_LBRACE = 18,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 19,                    /* RBRACE  */
  YYSYMBOL_COMMA = 20,                     /* COMMA  */
  YYSYMBOL_ASSIGN = 21,                    /* ASSIGN  */
  YYSYMBOL_SEMI = 22,                      /* SEMI  */
  YYSYMBOL_PLUS = 23,                      /* PLUS  */
  YYSYMBOL_MINUS = 24,                     /* MINUS  */
  YYSYMBOL_STAR = 25,                      /* STAR  */
  YYSYMBOL_SLASH = 26,                     /* SLASH  */
  YYSYMBOL_EQ = 27,                        /* EQ  */
  YYSYMBOL_NEQ = 28,                       /* NEQ  */
  YYSYMBOL_GT = 29,                        /* GT  */
  YYSYMBOL_LT = 30,                        /* LT  */
  YYSYMBOL_GTE = 31,                       /* GTE  */
  YYSYMBOL_LTE = 32,                       /* LTE  */
  YYSYMBOL_AND = 33,                       /* AND  */
  YYSYMBOL_OR = 34,                        /* OR  */
  YYSYMBOL_NOT = 35,                       /* NOT  */
  YYSYMBOL_ARROW = 36,                     /* ARROW  */
  YYSYMBOL_NUMBER = 37,                    /* NUMBER  */
  YYSYMBOL_IDENT = 38,                     /* IDENT  */
  YYSYMBOL_FIM = 39,                       /* FIM  */
  YYSYMBOL_NEWLINE = 40,                   /* NEWLINE  */
  YYSYMBOL_INVALID = 41,                   /* INVALID  */
  YYSYMBOL_UMINUS = 42,                    /* UMINUS  */
  YYSYMBOL_YYACCEPT = 43,                  /* $accept  */
  YYSYMBOL_program = 44,                   /* program  */
  YYSYMBOL_45_1 = 45,                      /* $@1  */
  YYSYMBOL_statements = 46,                /* statements  */
  YYSYMBOL_statement = 47,                 /* statement  */
  YYSYMBOL_terminator = 48,                /* terminator  */
  YYSYMBOL_assignment = 49,                /* assignment  */
  YYSYMBOL_if_stmt = 50,                   /* if_stmt  */
  YYSYMBOL_condition_set = 51,             /* condition_set  */
  YYSYMBOL_then_start = 52,                /* then_start  */
  YYSYMBOL_then_end = 53,                  /* then_end  */
  YYSYMBOL_else_start = 54,                /* else_start  */
  YYSYMBOL_else_end = 55,                  /* else_end  */
  YYSYMBOL_while_stmt = 56,                /* while_stmt  */
  YYSYMBOL_while_condition_start = 57,     /* while_condition_start  */
  YYSYMBOL_while_condition_end = 58,       /* while_condition_end  */
  YYSYMBOL_while_block_start = 59,         /* while_block_start  */
  YYSYMBOL_while_block = 60,               /* while_block  */
  YYSYMBOL_while_block_end = 61,           /* while_block_end  */
  YYSYMBOL_block = 62,                     /* block  */
  YYSYMBOL_command = 63,                   /* command  */
  YYSYMBOL_color = 64,                     /* color  */
  YYSYMBOL_sensor = 65,                    /* sensor  */
  YYSYMBOL_condition = 66,                 /* condition  */
  YYSYMBOL_logic_or = 67,                  /* logic_or  */
  YYSYMBOL_logic_and = 68,                 /* logic_and  */
  YYSYMBOL_rel_condition = 69,             /* rel_condition  */
  YYSYMBOL_70_2 = 70,                      /* $@2  */
  YYSYMBOL_71_3 = 71,                      /* $@3  */
  YYSYMBOL_72_4 = 72,                      /* $@4  */
  YYSYMBOL_73_5 = 73,                      /* $@5  */
  YYSYMBOL_74_6 = 74,                      /* $@6  */
  YYSYMBOL_75_7 = 75,                      /* $@7  */
  YYSYMBOL_expression = 76,                /* expression  */
  YYSYMBOL_77_8 = 77,                      /* $@8  */
  YYSYMBOL_78_9 = 78,                      /* $@9  */
  YYSYMBOL_79_10 = 79,                     /* $@10  */
  YYSYMBOL_80_11 = 80,                     /* $@11  */
  YYSYMBOL_term = 81                       /* term  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   159

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  43
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  39
/* YYNRULES -- Number of rules.  */
#define YYNRULES  71
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  126

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   297


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   236,   236,   236,   251,   255,   256,   260,   261,   262,
     263,   264,   265,   269,   270,   274,   288,   299,   318,   322,
     336,   351,   370,   377,   385,   396,   413,   420,   427,   446,
     450,   462,   474,   490,   502,   503,   504,   508,   509,   510,
     514,   529,   533,   568,   573,   608,   608,   629,   629,   646,
     646,   663,   663,   680,   680,   685,   685,   690,   697,   701,
     702,   702,   710,   710,   718,   718,   726,   726,   737,   742,
     749,   756
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "IF", "ELSE", "WHILE",
  "MUDAR", "PISCAR", "LER", "ESPERAR", "VERDE", "AMARELO", "VERMELHO",
  "HORARIO", "DURACAO", "FLUXO", "LPAREN", "RPAREN", "LBRACE", "RBRACE",
  "COMMA", "ASSIGN", "SEMI", "PLUS", "MINUS", "STAR", "SLASH", "EQ", "NEQ",
  "GT", "LT", "GTE", "LTE", "AND", "OR", "NOT", "ARROW", "NUMBER", "IDENT",
  "FIM", "NEWLINE", "INVALID", "UMINUS", "$accept", "program", "$@1",
  "statements", "statement", "terminator", "assignment", "if_stmt",
  "condition_set", "then_start", "then_end", "else_start", "else_end",
  "while_stmt", "while_condition_start", "while_condition_end",
  "while_block_start", "while_block", "while_block_end", "block",
  "command", "color", "sensor", "condition", "logic_or", "logic_and",
  "rel_condition", "$@2", "$@3", "$@4", "$@5", "$@6", "$@7", "expression",
  "$@8", "$@9", "$@10", "$@11", "term", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-83)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-67)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       5,   -83,    22,    77,   -83,    20,    21,    23,    25,    33,
      38,    77,   -83,    40,   -83,    77,   -83,   -83,     2,   -83,
     -83,   -83,     2,     9,   -83,    93,    93,   130,    74,    12,
      74,   -83,   -83,   -83,     9,    74,     9,   -83,   -83,    51,
     -83,    47,    41,   -83,   106,   -83,     9,   -83,   -83,   -83,
      79,    67,   -83,   -83,   -83,    80,    74,    31,   -83,   116,
      92,    96,   -83,   -83,   -83,     9,     9,    61,    65,    82,
      85,    91,    98,   113,   122,   123,   121,   132,   -83,   -83,
      74,   114,    34,   -83,   -83,   -83,   133,    41,   -83,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,   -83,
      76,   115,   -83,   116,   116,   116,   116,   116,   116,   -83,
     -83,   -83,   -83,   134,   -83,   -83,   150,    77,   -83,   -83,
      70,   -83,   133,   -83,   -83,   -83
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     4,     0,     0,     1,     0,     0,     0,     0,     0,
       0,     0,    13,     0,    14,     3,     5,    12,     0,     9,
      10,    11,     0,     0,    24,     0,     0,     0,     0,     0,
       0,     6,     7,     8,     0,     0,     0,    68,    69,     0,
      18,    40,    41,    43,    45,    59,     0,    34,    35,    36,
       0,     0,    37,    38,    39,     0,     0,    60,    29,    15,
       0,    45,    70,    57,    19,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    25,    30,
       0,     0,    60,    33,    58,    71,     0,    42,    44,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    26,
      60,     0,    20,    46,    48,    50,    52,    54,    56,    61,
      63,    65,    67,     0,    31,    32,    16,     0,    28,    21,
       0,    23,     0,    27,    22,    17
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -83,   -83,   -83,   -11,   -13,    10,   -83,   -83,   -83,   -83,
     -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -83,   -82,
     -83,   129,   -83,    -8,   -83,    94,   -31,   -83,   -83,   -83,
     -83,   -83,   -83,   -27,   -83,   -83,   -83,   -83,   -26
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     2,     3,    15,    16,    17,    18,    19,    39,    86,
     116,   122,   125,    20,    46,    77,   113,   118,   121,    21,
      22,    50,    55,    40,    41,    42,    43,    67,    68,    69,
      70,    71,    72,    44,    73,    74,    75,    76,    45
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      29,    57,    31,    59,   102,    63,     1,    61,    -2,    62,
      -2,    -2,    -2,    -2,    -2,     5,    31,     6,     7,     8,
       9,    10,     4,    -2,    12,    34,    60,    -2,    32,    82,
      11,    58,    33,    35,    12,    88,    23,    24,    78,    25,
     124,    26,    14,    -2,    36,    -2,    37,    38,    83,    27,
      13,    85,    14,   100,    28,   -62,   -64,   -66,   -62,   -64,
     -66,    30,   103,   104,   105,   106,   107,   108,    64,   109,
     110,   111,   112,     5,    66,     6,     7,     8,     9,    10,
       5,    65,     6,     7,     8,     9,    10,    80,    11,   123,
      56,    89,    12,   114,    90,    11,    79,    81,    35,    12,
     -62,   -64,   -66,    47,    48,    49,   120,    31,    13,    84,
      14,    37,    38,    85,    91,    13,    92,    14,    93,   -60,
     -62,   -64,   -66,   -53,   -55,   -47,    94,   -51,   -49,   -60,
     -62,   -64,   -66,   -53,   -55,   -47,    95,   -51,   -49,   -60,
     -62,   -64,   -66,    52,    53,    54,    96,    98,    97,    99,
     101,    11,   117,   115,   119,    51,     0,     0,     0,    87
};

static const yytype_int8 yycheck[] =
{
      11,    28,    15,    30,    86,    36,     1,    34,     3,    35,
       5,     6,     7,     8,     9,     3,    29,     5,     6,     7,
       8,     9,     0,    18,    22,    16,    34,    22,    18,    56,
      18,    19,    22,    24,    22,    66,    16,    16,    46,    16,
     122,    16,    40,    38,    35,    40,    37,    38,    17,    16,
      38,    17,    40,    80,    16,    24,    25,    26,    24,    25,
      26,    21,    89,    90,    91,    92,    93,    94,    17,    95,
      96,    97,    98,     3,    33,     5,     6,     7,     8,     9,
       3,    34,     5,     6,     7,     8,     9,    20,    18,    19,
      16,    30,    22,    17,    29,    18,    17,    17,    24,    22,
      24,    25,    26,    10,    11,    12,   117,   120,    38,    17,
      40,    37,    38,    17,    32,    38,    31,    40,    27,    23,
      24,    25,    26,    27,    28,    29,    28,    31,    32,    23,
      24,    25,    26,    27,    28,    29,    23,    31,    32,    23,
      24,    25,    26,    13,    14,    15,    24,    26,    25,    17,
      36,    18,    18,    38,     4,    26,    -1,    -1,    -1,    65
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     1,    44,    45,     0,     3,     5,     6,     7,     8,
       9,    18,    22,    38,    40,    46,    47,    48,    49,    50,
      56,    62,    63,    16,    16,    16,    16,    16,    16,    46,
      21,    47,    48,    48,    16,    24,    35,    37,    38,    51,
      66,    67,    68,    69,    76,    81,    57,    10,    11,    12,
      64,    64,    13,    14,    15,    65,    16,    76,    19,    76,
      66,    76,    81,    69,    17,    34,    33,    70,    71,    72,
      73,    74,    75,    77,    78,    79,    80,    58,    66,    17,
      20,    17,    76,    17,    17,    17,    52,    68,    69,    30,
      29,    32,    31,    27,    28,    23,    24,    25,    26,    17,
      76,    36,    62,    76,    76,    76,    76,    76,    76,    81,
      81,    81,    81,    59,    17,    38,    53,    18,    60,     4,
      46,    61,    54,    19,    62,    55
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    43,    45,    44,    44,    46,    46,    47,    47,    47,
      47,    47,    47,    48,    48,    49,    50,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    63,    63,    63,    64,    64,    64,    65,    65,    65,
      66,    67,    67,    68,    68,    70,    69,    71,    69,    72,
      69,    73,    69,    74,    69,    75,    69,    69,    69,    76,
      77,    76,    78,    76,    79,    76,    80,    76,    81,    81,
      81,    81
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     2,     2,     2,     1,
       1,     1,     1,     1,     1,     3,     7,    11,     1,     0,
       0,     0,     0,     8,     0,     1,     0,     3,     0,     3,
       4,     6,     6,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     1,     3,     0,     4,     0,     4,     0,
       4,     0,     4,     0,     4,     0,     4,     2,     3,     1,
       0,     4,     0,     4,     0,     4,     0,     4,     1,     1,
       2,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* $@1: %empty  */
#line 236 "parser.y"
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
#line 1437 "parser.tab.c"
    break;

  case 3: /* program: $@1 statements  */
#line 246 "parser.y"
               { 
        /* Adiciona HALT no final */
        vm_add_instruction(vm, VM_HALT, 0, 0, NULL);
        YYACCEPT; 
    }
#line 1447 "parser.tab.c"
    break;

  case 4: /* program: error  */
#line 251 "parser.y"
            { yyclearin; yyerrok; YYABORT; }
#line 1453 "parser.tab.c"
    break;

  case 15: /* assignment: IDENT ASSIGN expression  */
#line 274 "parser.y"
                            { 
        if (current_exec()) { 
            set_var((yyvsp[-2].str), (yyvsp[0].num));
            printf("  -> Variável '%s' = %d\n", (yyvsp[-2].str), (yyvsp[0].num));
        }
        /* Sempre gera assembly, independente de current_exec() */
        /* Gera assembly: expression já deixou resultado em R0 */
        int addr = vm_alloc_var(get_vm(), (yyvsp[-2].str));
        vm_add_instruction(get_vm(), VM_STORE, addr, 0, NULL);
        free((yyvsp[-2].str));
    }
#line 1469 "parser.tab.c"
    break;

  case 16: /* if_stmt: IF LPAREN condition_set RPAREN then_start block then_end  */
#line 289 "parser.y"
    {
        /* then_start ($5) gerou JZ com false_label, então geramos o label aqui */
        char *false_label = (char*)(yyvsp[-2].num); /* $5 é o then_start que retornou o label */
        if (false_label) {
            vm_add_instruction(get_vm(), VM_NOP, 0, 0, false_label);
            free(false_label);
        }
        push_exec(current_exec() && last_cond);
        pop_exec();
    }
#line 1484 "parser.tab.c"
    break;

  case 17: /* if_stmt: IF LPAREN condition_set RPAREN then_start block then_end ELSE else_start block else_end  */
#line 300 "parser.y"
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
#line 1504 "parser.tab.c"
    break;

  case 18: /* condition_set: condition  */
#line 318 "parser.y"
              { (yyval.num) = (yyvsp[0].num); last_cond = last_cond_value; }
#line 1510 "parser.tab.c"
    break;

  case 19: /* then_start: %empty  */
#line 322 "parser.y"
    { 
        /* condition deixou resultado na stack (via logic_or) */
        /* Restaura resultado da stack para R0 antes do JZ */
        vm_add_instruction(get_vm(), VM_POP, 0, 0, NULL); /* Restaura resultado para R0 */
        /* Cria um novo label para o JZ */
        char *false_label = new_label();
        push_else_label(strdup(false_label)); /* Armazena na pilha para uso no else_start */
        vm_add_instruction(get_vm(), VM_JZ, 0, 0, false_label);
        (yyval.num) = (int)false_label; /* Retorna o label como ponteiro */
        push_exec(current_exec() && last_cond);
    }
#line 1526 "parser.tab.c"
    break;

  case 20: /* then_end: %empty  */
#line 336 "parser.y"
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
        (yyval.num) = 0; 
    }
#line 1543 "parser.tab.c"
    break;

  case 21: /* else_start: %empty  */
#line 351 "parser.y"
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
        (yyval.num) = 0;
        push_exec(current_exec() && !last_cond); 
    }
#line 1564 "parser.tab.c"
    break;

  case 22: /* else_end: %empty  */
#line 370 "parser.y"
    { 
        pop_exec(); 
        (yyval.num) = 0; 
    }
#line 1573 "parser.tab.c"
    break;

  case 23: /* while_stmt: WHILE LPAREN while_condition_start while_condition_end RPAREN while_block_start while_block while_block_end  */
#line 378 "parser.y"
    {
        /* Para assembly: código de loop já foi gerado com labels e jumps */
        (yyval.num) = 0;
    }
#line 1582 "parser.tab.c"
    break;

  case 24: /* while_condition_start: %empty  */
#line 385 "parser.y"
    {
        /* Para assembly: cria label de início do loop */
        char *loop_start_label = new_label();
        vm_add_instruction(get_vm(), VM_NOP, 0, 0, loop_start_label);
        /* Armazena o label para uso no while_block_end */
        push_else_label(loop_start_label); /* Reutiliza a pilha de else para armazenar loop_start */
        (yyval.num) = 0;
    }
#line 1595 "parser.tab.c"
    break;

  case 25: /* while_condition_end: condition  */
#line 397 "parser.y"
    {
        /* Para assembly: avalia condição e pula para fim se falsa */
        /* condition deixou resultado na stack */
        vm_add_instruction(get_vm(), VM_POP, 0, 0, NULL); /* Restaura resultado para R0 */
        char *loop_end_label = new_label();
        vm_add_instruction(get_vm(), VM_JZ, 0, 0, loop_end_label); /* Se falso, sai do loop */
        /* Armazena o label do fim para uso no while_block_end */
        push_end_label(loop_end_label);
        /* Para execução interpretada: armazena o valor da condição */
        last_cond_value = ((yyvsp[0].num) != 0) ? 1 : 0;
        push_exec(current_exec() && last_cond_value);
        (yyval.num) = (yyvsp[0].num);
    }
#line 1613 "parser.tab.c"
    break;

  case 26: /* while_block_start: %empty  */
#line 413 "parser.y"
    {
        /* Para execução interpretada: início do bloco */
        (yyval.num) = 0;
    }
#line 1622 "parser.tab.c"
    break;

  case 27: /* while_block: LBRACE statements RBRACE  */
#line 421 "parser.y"
    {
        (yyval.num) = 0;
    }
#line 1630 "parser.tab.c"
    break;

  case 28: /* while_block_end: %empty  */
#line 427 "parser.y"
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
        (yyval.num) = 0;
    }
#line 1651 "parser.tab.c"
    break;

  case 30: /* command: MUDAR LPAREN color RPAREN  */
#line 450 "parser.y"
                              { 
        if (current_exec()) { 
            executar_comando("mudar", (yyvsp[-1].str), 0);
        }
        /* Sempre gera assembly, independente de current_exec() */
        int cor = 0;
        if (strcmp((yyvsp[-1].str), "verde") == 0) cor = 0;
        else if (strcmp((yyvsp[-1].str), "amarelo") == 0) cor = 1;
        else cor = 2;
        vm_add_instruction(get_vm(), VM_MUDAR, cor, 0, NULL);
        free((yyvsp[-1].str)); 
    }
#line 1668 "parser.tab.c"
    break;

  case 31: /* command: PISCAR LPAREN color COMMA expression RPAREN  */
#line 462 "parser.y"
                                                  { 
        if (current_exec()) { 
            executar_comando("piscar", (yyvsp[-3].str), (yyvsp[-1].num));
            /* Gera assembly: expression já deixou resultado em R0 */
            int cor = 0;
            if (strcmp((yyvsp[-3].str), "verde") == 0) cor = 0;
            else if (strcmp((yyvsp[-3].str), "amarelo") == 0) cor = 1;
            else cor = 2;
            vm_add_instruction(get_vm(), VM_PISCAR, cor, (yyvsp[-1].num), NULL);
        }
        free((yyvsp[-3].str)); 
    }
#line 1685 "parser.tab.c"
    break;

  case 32: /* command: LER LPAREN sensor RPAREN ARROW IDENT  */
#line 474 "parser.y"
                                           { 
        if (current_exec()) { 
            int v = ler_sensor((yyvsp[-3].str));
            set_var((yyvsp[0].str), v);
            /* Gera assembly */
            int sensor_id = 0;
            if (strcmp((yyvsp[-3].str), "horario") == 0) sensor_id = SENSOR_HORARIO;
            else if (strcmp((yyvsp[-3].str), "duracao") == 0) sensor_id = SENSOR_DURACAO;
            else if (strcmp((yyvsp[-3].str), "fluxo") == 0) sensor_id = SENSOR_FLUXO;
            vm_add_instruction(get_vm(), VM_READ_SENSOR, sensor_id, 0, NULL);
            int addr = vm_alloc_var(get_vm(), (yyvsp[0].str));
            vm_add_instruction(get_vm(), VM_STORE, addr, 0, NULL);
        }
        free((yyvsp[-3].str)); 
        free((yyvsp[0].str)); 
    }
#line 1706 "parser.tab.c"
    break;

  case 33: /* command: ESPERAR LPAREN expression RPAREN  */
#line 490 "parser.y"
                                       { 
        if (current_exec()) { 
            executar_comando("esperar", "", (yyvsp[-1].num));
        }
        /* Sempre gera assembly, independente de current_exec() */
        /* Gera assembly: ESPERAR usa o valor em R0 que já foi carregado pela expression */
        /* O valor já está em R0, então ESPERAR não precisa de argumento */
        vm_add_instruction(get_vm(), VM_ESPERAR, 0, 0, NULL);
    }
#line 1720 "parser.tab.c"
    break;

  case 34: /* color: VERDE  */
#line 502 "parser.y"
          { (yyval.str) = strdup("verde"); }
#line 1726 "parser.tab.c"
    break;

  case 35: /* color: AMARELO  */
#line 503 "parser.y"
              { (yyval.str) = strdup("amarelo"); }
#line 1732 "parser.tab.c"
    break;

  case 36: /* color: VERMELHO  */
#line 504 "parser.y"
               { (yyval.str) = strdup("vermelho"); }
#line 1738 "parser.tab.c"
    break;

  case 37: /* sensor: HORARIO  */
#line 508 "parser.y"
            { (yyval.str) = strdup("horario"); }
#line 1744 "parser.tab.c"
    break;

  case 38: /* sensor: DURACAO  */
#line 509 "parser.y"
              { (yyval.str) = strdup("duracao"); }
#line 1750 "parser.tab.c"
    break;

  case 39: /* sensor: FLUXO  */
#line 510 "parser.y"
            { (yyval.str) = strdup("fluxo"); }
#line 1756 "parser.tab.c"
    break;

  case 40: /* condition: logic_or  */
#line 515 "parser.y"
    {
        /* logic_or deixou resultado na stack (do logic_and) */
        /* Não fazemos POP aqui, o resultado fica na stack */
        /* O then_start fará POP antes do JZ */
        /* Armazena o valor booleano da condição para uso em last_cond */
        last_cond_value = ((yyvsp[0].num) != 0) ? 1 : 0;
        /* Gera label para pular se falso */
        char *false_label = new_label();
        /* JZ será gerado pelo then_start, então apenas retornamos o label */
        (yyval.num) = (int)false_label;
    }
#line 1772 "parser.tab.c"
    break;

  case 41: /* logic_or: logic_and  */
#line 529 "parser.y"
              { 
        (yyval.num) = (yyvsp[0].num);
        /* logic_and já deixou resultado na stack, não precisa fazer PUSH novamente */
    }
#line 1781 "parser.tab.c"
    break;

  case 42: /* logic_or: logic_or OR logic_and  */
#line 533 "parser.y"
                            { 
        /* Para execução interpretada: calcula OR lógico corretamente */
        /* $1 e $3 são valores semânticos (0 ou 1) */
        /* $1 é o resultado do logic_or anterior, $3 é o resultado do logic_and */
        int val1 = ((yyvsp[-2].num) != 0) ? 1 : 0;
        int val3 = ((yyvsp[0].num) != 0) ? 1 : 0;
        (yyval.num) = (val1 || val3) ? 1 : 0;
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
#line 1818 "parser.tab.c"
    break;

  case 43: /* logic_and: rel_condition  */
#line 568 "parser.y"
                  { 
        (yyval.num) = (yyvsp[0].num);
        /* Resultado está em R0, salva na stack para possível uso em AND seguinte */
        vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL);
    }
#line 1828 "parser.tab.c"
    break;

  case 44: /* logic_and: logic_and AND rel_condition  */
#line 573 "parser.y"
                                  { 
        /* Para execução interpretada: calcula AND lógico corretamente */
        /* $1 e $3 são valores semânticos (0 ou 1) */
        int val1 = ((yyvsp[-2].num) != 0) ? 1 : 0;
        int val3 = ((yyvsp[0].num) != 0) ? 1 : 0;
        (yyval.num) = (val1 && val3) ? 1 : 0;
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
#line 1865 "parser.tab.c"
    break;

  case 45: /* $@2: %empty  */
#line 608 "parser.y"
               { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); }
#line 1871 "parser.tab.c"
    break;

  case 46: /* rel_condition: expression $@2 LT expression  */
#line 608 "parser.y"
                                                                                    { 
        /* Para execução interpretada: $1 é expression1, $4 é expression2 */
        int expr1 = (yyvsp[-3].num);
        int expr2 = (yyvsp[0].num);
        (yyval.num) = (expr1 < expr2) ? 1 : 0;
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
#line 1897 "parser.tab.c"
    break;

  case 47: /* $@3: %empty  */
#line 629 "parser.y"
                 { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); }
#line 1903 "parser.tab.c"
    break;

  case 48: /* rel_condition: expression $@3 GT expression  */
#line 629 "parser.y"
                                                                                      { 
        /* Para execução interpretada: $1 é expression1, $4 é expression2 */
        int expr1 = (yyvsp[-3].num);
        int expr2 = (yyvsp[0].num);
        (yyval.num) = (expr1 > expr2) ? 1 : 0;
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
#line 1925 "parser.tab.c"
    break;

  case 49: /* $@4: %empty  */
#line 646 "parser.y"
                 { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); }
#line 1931 "parser.tab.c"
    break;

  case 50: /* rel_condition: expression $@4 LTE expression  */
#line 646 "parser.y"
                                                                                       { 
        /* Para execução interpretada: $1 é expression1, $4 é expression2 */
        int expr1 = (yyvsp[-3].num);
        int expr2 = (yyvsp[0].num);
        (yyval.num) = (expr1 <= expr2) ? 1 : 0;
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
#line 1953 "parser.tab.c"
    break;

  case 51: /* $@5: %empty  */
#line 663 "parser.y"
                 { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); }
#line 1959 "parser.tab.c"
    break;

  case 52: /* rel_condition: expression $@5 GTE expression  */
#line 663 "parser.y"
                                                                                       { 
        /* Para execução interpretada: $1 é expression1, $4 é expression2 */
        int expr1 = (yyvsp[-3].num);
        int expr2 = (yyvsp[0].num);
        (yyval.num) = (expr1 >= expr2) ? 1 : 0;
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
#line 1981 "parser.tab.c"
    break;

  case 53: /* $@6: %empty  */
#line 680 "parser.y"
                 { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); }
#line 1987 "parser.tab.c"
    break;

  case 54: /* rel_condition: expression $@6 EQ expression  */
#line 680 "parser.y"
                                                                                      { 
        (yyval.num) = ((yyvsp[-3].num) == (yyvsp[0].num));
        vm_add_instruction(get_vm(), VM_EQ, 0, 0, NULL);
        /* EQ já deixa 1 ou 0 em R0 */
    }
#line 1997 "parser.tab.c"
    break;

  case 55: /* $@7: %empty  */
#line 685 "parser.y"
                 { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); }
#line 2003 "parser.tab.c"
    break;

  case 56: /* rel_condition: expression $@7 NEQ expression  */
#line 685 "parser.y"
                                                                                       { 
        (yyval.num) = ((yyvsp[-3].num) != (yyvsp[0].num));
        vm_add_instruction(get_vm(), VM_NE, 0, 0, NULL);
        /* NE já deixa 1 ou 0 em R0 */
    }
#line 2013 "parser.tab.c"
    break;

  case 57: /* rel_condition: NOT rel_condition  */
#line 690 "parser.y"
                        { 
        (yyval.num) = !(yyvsp[0].num);
        /* Inverte o resultado em R0: se 1 vira 0, se 0 vira 1 */
        vm_add_instruction(get_vm(), VM_LOAD, 1, 0, NULL);
        vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL);
        vm_add_instruction(get_vm(), VM_SUB, 0, 0, NULL);
    }
#line 2025 "parser.tab.c"
    break;

  case 58: /* rel_condition: LPAREN condition RPAREN  */
#line 697 "parser.y"
                              { (yyval.num) = (yyvsp[-1].num); }
#line 2031 "parser.tab.c"
    break;

  case 59: /* expression: term  */
#line 701 "parser.y"
         { (yyval.num) = (yyvsp[0].num); }
#line 2037 "parser.tab.c"
    break;

  case 60: /* $@8: %empty  */
#line 702 "parser.y"
                 { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); }
#line 2043 "parser.tab.c"
    break;

  case 61: /* expression: expression $@8 PLUS term  */
#line 702 "parser.y"
                                                                                  { 
        /* Para execução interpretada: $1 é expression, $4 é term */
        (yyval.num) = (yyvsp[-3].num) + (yyvsp[0].num);
        /* Gera assembly: soma */
        /* expression foi salvo na stack pela mid-rule action */
        /* term está em R0 */
        vm_add_instruction(get_vm(), VM_ADD, 0, 0, NULL);
    }
#line 2056 "parser.tab.c"
    break;

  case 62: /* $@9: %empty  */
#line 710 "parser.y"
                 { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); }
#line 2062 "parser.tab.c"
    break;

  case 63: /* expression: expression $@9 MINUS term  */
#line 710 "parser.y"
                                                                                   { 
        /* Para execução interpretada: $1 é expression, $4 é term */
        (yyval.num) = (yyvsp[-3].num) - (yyvsp[0].num);
        /* Gera assembly: subtrai */
        /* expression foi salvo na stack pela mid-rule action */
        /* term está em R0 */
        vm_add_instruction(get_vm(), VM_SUB, 0, 0, NULL);
    }
#line 2075 "parser.tab.c"
    break;

  case 64: /* $@10: %empty  */
#line 718 "parser.y"
                 { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); }
#line 2081 "parser.tab.c"
    break;

  case 65: /* expression: expression $@10 STAR term  */
#line 718 "parser.y"
                                                                                  { 
        /* Para execução interpretada: $1 é expression, $4 é term */
        (yyval.num) = (yyvsp[-3].num) * (yyvsp[0].num);
        /* Gera assembly: multiplica */
        /* expression foi salvo na stack pela mid-rule action */
        /* term está em R0 */
        vm_add_instruction(get_vm(), VM_MUL, 0, 0, NULL);
    }
#line 2094 "parser.tab.c"
    break;

  case 66: /* $@11: %empty  */
#line 726 "parser.y"
                 { vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL); }
#line 2100 "parser.tab.c"
    break;

  case 67: /* expression: expression $@11 SLASH term  */
#line 726 "parser.y"
                                                                                   { 
        /* Para execução interpretada: $1 é expression, $4 é term */
        (yyval.num) = ((yyvsp[0].num) != 0) ? ((yyvsp[-3].num) / (yyvsp[0].num)) : 0;
        /* Gera assembly: divide */
        /* expression foi salvo na stack pela mid-rule action */
        /* term está em R0 */
        vm_add_instruction(get_vm(), VM_DIV, 0, 0, NULL);
    }
#line 2113 "parser.tab.c"
    break;

  case 68: /* term: NUMBER  */
#line 737 "parser.y"
           { 
        (yyval.num) = (yyvsp[0].num);
        /* Gera assembly: carrega número em R0 */
        vm_add_instruction(get_vm(), VM_LOAD, (yyvsp[0].num), 0, NULL);
    }
#line 2123 "parser.tab.c"
    break;

  case 69: /* term: IDENT  */
#line 742 "parser.y"
            { 
        (yyval.num) = get_var((yyvsp[0].str));
        /* Gera assembly: carrega variável da memória */
        int addr = vm_alloc_var(get_vm(), (yyvsp[0].str));
        vm_add_instruction(get_vm(), VM_LOADM, addr, 0, NULL);
        free((yyvsp[0].str)); 
    }
#line 2135 "parser.tab.c"
    break;

  case 70: /* term: MINUS term  */
#line 749 "parser.y"
                              { 
        (yyval.num) = -(yyvsp[0].num);
        /* Gera assembly: nega o valor em R0 */
        vm_add_instruction(get_vm(), VM_LOAD, 0, 0, NULL);
        vm_add_instruction(get_vm(), VM_PUSH, 0, 0, NULL);
        vm_add_instruction(get_vm(), VM_SUB, 0, 0, NULL);
    }
#line 2147 "parser.tab.c"
    break;

  case 71: /* term: LPAREN expression RPAREN  */
#line 756 "parser.y"
                               { (yyval.num) = (yyvsp[-1].num); }
#line 2153 "parser.tab.c"
    break;


#line 2157 "parser.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 759 "parser.y"


void yyerror(const char *s) {
    fprintf(stderr, "ERRO SINTÁTICO (linha %d): %s\n", yylineno, s);
}
