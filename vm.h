#ifndef VM_H
#define VM_H

#include <stdio.h>

/* Sensores */
#define SENSOR_HORARIO 0
#define SENSOR_DURACAO 1
#define SENSOR_FLUXO 2

/* Opcodes da VM */
typedef enum {
    VM_LOAD,
    VM_LOADM,
    VM_STORE,
    VM_ADD,
    VM_SUB,
    VM_MUL,
    VM_DIV,
    VM_READ_SENSOR,
    VM_JMP,
    VM_JZ,
    VM_JNZ,
    VM_JLT,
    VM_JGT,
    VM_JLE,
    VM_JGE,
    VM_CMP,
    VM_EQ,
    VM_NE,
    VM_MUDAR,
    VM_PISCAR,
    VM_ESPERAR,
    VM_PUSH,
    VM_POP,
    VM_HALT,
    VM_NOP
} VM_Opcode;

/* Estrutura de uma instrução */
typedef struct {
    VM_Opcode op;
    int arg1;
    int arg2;
    char *label;  /* Para jumps e labels */
} VM_Instruction;

/* Estado da VM */
#define VM_MEM_SIZE 256
#define VM_STACK_SIZE 256
#define R0 0
#define R1 1

typedef struct {
    VM_Instruction *code;
    int code_size;
    int code_capacity;
    int memory[VM_MEM_SIZE];
    int stack[VM_STACK_SIZE];
    int sp;  /* Stack pointer */
    int regs[2];  /* R0, R1 */
    int pc;  /* Program counter */
    int cor_atual;  /* 0=VERDE, 1=AMARELO, 2=VERMELHO */
} VM_State;

/* Funções da VM */
VM_State* vm_create(void);
void vm_destroy(VM_State *vm);
void vm_add_instruction(VM_State *vm, VM_Opcode op, int arg1, int arg2, const char *label);
int vm_alloc_var(VM_State *vm, const char *name);
VM_State* vm_load_from_asm(const char *filename);
void vm_execute(VM_State *vm);
void vm_print_code(VM_State *vm, FILE *out);

#endif

