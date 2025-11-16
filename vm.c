#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Tabela de símbolos para variáveis */
typedef struct {
    char *name;
    int addr;
} VarEntry;

static VarEntry var_table[256];
static int var_count = 0;
static int next_addr = 0;

VM_State* vm_create(void) {
    VM_State *vm = (VM_State*)calloc(1, sizeof(VM_State));
    vm->code_capacity = 1024;
    vm->code = (VM_Instruction*)calloc(vm->code_capacity, sizeof(VM_Instruction));
    vm->code_size = 0;
    vm->sp = -1;
    vm->pc = 0;
    vm->cor_atual = 0; // VERDE
    var_count = 0;
    next_addr = 0;
    return vm;
}

void vm_destroy(VM_State *vm) {
    if (vm) {
        for (int i = 0; i < vm->code_size; i++) {
            if (vm->code[i].label) {
                free(vm->code[i].label);
            }
        }
        free(vm->code);
        for (int i = 0; i < var_count; i++) {
            free(var_table[i].name);
        }
        free(vm);
    }
}

int vm_alloc_var(VM_State *vm, const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_table[i].name, name) == 0) {
            return var_table[i].addr;
        }
    }
    if (var_count < 256 && next_addr < VM_MEM_SIZE) {
        var_table[var_count].name = strdup(name);
        var_table[var_count].addr = next_addr;
        var_count++;
        return next_addr++;
    }
    return -1;
}

void vm_add_instruction(VM_State *vm, VM_Opcode op, int arg1, int arg2, const char *label) {
    if (vm->code_size >= vm->code_capacity) {
        vm->code_capacity *= 2;
        vm->code = (VM_Instruction*)realloc(vm->code, vm->code_capacity * sizeof(VM_Instruction));
    }
    VM_Instruction *instr = &vm->code[vm->code_size];
    instr->op = op;
    instr->arg1 = arg1;
    instr->arg2 = arg2;
    instr->label = label ? strdup(label) : NULL;
    vm->code_size++;
}

int vm_get_sensor(int sensor_id) {
    switch (sensor_id) {
        case SENSOR_HORARIO: {
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            return tm->tm_hour;
        }
        case SENSOR_DURACAO: {
            return rand() % 10 + 1; /* Simula duração entre 1 e 10 segundos */
        }
        case SENSOR_FLUXO: {
            return rand() % 50 + 1;
        }
        default:
            return 0;
    }
}

const char* vm_opcode_name(VM_Opcode op) {
    switch (op) {
        case VM_LOAD: return "LOAD";
        case VM_LOADM: return "LOADM";
        case VM_STORE: return "STORE";
        case VM_ADD: return "ADD";
        case VM_SUB: return "SUB";
        case VM_MUL: return "MUL";
        case VM_DIV: return "DIV";
        case VM_READ_SENSOR: return "READ_SENSOR";
        case VM_JMP: return "JMP";
        case VM_JZ: return "JZ";
        case VM_JNZ: return "JNZ";
        case VM_JLT: return "JLT";
        case VM_JGT: return "JGT";
        case VM_JLE: return "JLE";
        case VM_JGE: return "JGE";
        case VM_CMP: return "CMP";
        case VM_EQ: return "EQ";
        case VM_NE: return "NE";
        case VM_MUDAR: return "MUDAR";
        case VM_PISCAR: return "PISCAR";
        case VM_ESPERAR: return "ESPERAR";
        case VM_PUSH: return "PUSH";
        case VM_POP: return "POP";
        case VM_HALT: return "HALT";
        case VM_NOP: return "NOP";
        default: return "UNKNOWN";
    }
}

/* Resolve labels para endereços */
static void vm_resolve_labels(VM_State *vm) {
    typedef struct {
        char *name;
        int addr;
    } LabelEntry;
    
    LabelEntry labels[256];
    int label_count = 0;
    
    /* Primeira passada: encontra todos os labels */
    for (int i = 0; i < vm->code_size; i++) {
        if (vm->code[i].label && vm->code[i].op == VM_NOP) {
            labels[label_count].name = vm->code[i].label;
            labels[label_count].addr = i;
            label_count++;
        }
    }
    
    /* Segunda passada: substitui labels por endereços */
    for (int i = 0; i < vm->code_size; i++) {
        VM_Instruction *instr = &vm->code[i];
        if (instr->label && (instr->op == VM_JMP || instr->op == VM_JZ || 
            instr->op == VM_JNZ || instr->op == VM_JLT || instr->op == VM_JGT ||
            instr->op == VM_JLE || instr->op == VM_JGE)) {
            int found = 0;
            for (int j = 0; j < label_count; j++) {
                if (strcmp(instr->label, labels[j].name) == 0) {
                    instr->arg1 = labels[j].addr;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "Erro: Label '%s' não encontrado na linha %d\n", instr->label, i);
                /* Se não encontrou, pula para HALT ou fim do programa */
                instr->arg1 = vm->code_size - 1;
            }
        }
    }
}

void vm_execute(VM_State *vm) {
    vm_resolve_labels(vm);
    
    const char *cores[] = {"VERDE", "AMARELO", "VERMELHO"};
    int max_instructions = 100000; /* Limite para evitar loops infinitos */
    int instruction_count = 0;
    
    while (vm->pc < vm->code_size && instruction_count < max_instructions) {
        instruction_count++;
        VM_Instruction *instr = &vm->code[vm->pc];
        
        switch (instr->op) {
            case VM_LOAD:
                vm->regs[R0] = instr->arg1;
                vm->pc++;
                break;
                
            case VM_LOADM:
                if (instr->arg1 >= 0 && instr->arg1 < VM_MEM_SIZE) {
                    vm->regs[R0] = vm->memory[instr->arg1];
                }
                vm->pc++;
                break;
                
            case VM_STORE:
                if (instr->arg1 >= 0 && instr->arg1 < VM_MEM_SIZE) {
                    vm->memory[instr->arg1] = vm->regs[R0];
                }
                vm->pc++;
                break;
                
            case VM_ADD:
                if (vm->sp >= 0) {
                    vm->regs[R0] = vm->regs[R0] + vm->stack[vm->sp--];
                } else {
                    vm->regs[R0] = vm->regs[R0] + vm->regs[R1];
                }
                vm->pc++;
                break;
                
            case VM_SUB:
                if (vm->sp >= 0) {
                    int top = vm->stack[vm->sp--];
                    vm->regs[R0] = top - vm->regs[R0];
                } else {
                    vm->regs[R0] = vm->regs[R0] - vm->regs[R1];
                }
                vm->pc++;
                break;
                
            case VM_MUL:
                if (vm->sp >= 0) {
                    vm->regs[R0] = vm->regs[R0] * vm->stack[vm->sp--];
                } else {
                    vm->regs[R0] = vm->regs[R0] * vm->regs[R1];
                }
                vm->pc++;
                break;
                
            case VM_DIV:
                if (vm->sp >= 0) {
                    int top = vm->stack[vm->sp--];
                    if (vm->regs[R0] != 0) {
                        vm->regs[R0] = top / vm->regs[R0];
                    } else {
                        vm->regs[R0] = 0;
                    }
                } else {
                    if (vm->regs[R1] != 0) {
                        vm->regs[R0] = vm->regs[R0] / vm->regs[R1];
                    } else {
                        vm->regs[R0] = 0;
                    }
                }
                vm->pc++;
                break;
                
            case VM_READ_SENSOR:
                vm->regs[R0] = vm_get_sensor(instr->arg1);
                printf("  -> Sensor %d lido: %d\n", instr->arg1, vm->regs[R0]);
                vm->pc++;
                break;
                
            case VM_MUDAR:
                vm->cor_atual = instr->arg1;
                printf("  -> Mudando semáforo para %s\n", cores[instr->arg1]);
                vm->pc++;
                break;
                
            case VM_PISCAR:
                printf("  -> Piscando %s por %d vezes\n", cores[instr->arg1], instr->arg2);
                for (int i = 0; i < instr->arg2; i++) {
                    printf("    [%s]", cores[instr->arg1]);
                    fflush(stdout);
                    usleep(200000);
                    printf(" [VERMELHO]");
                    fflush(stdout);
                    usleep(200000);
                }
                printf("\n");
                vm->pc++;
                break;
                
            case VM_ESPERAR:
                {
                    int segundos = vm->regs[R0];
                    if (segundos < 0) segundos = 0;
                    if (segundos > 60) segundos = 60; /* Limita a 60 segundos para testes */
                    printf("  -> Esperando %d segundos...\n", segundos);
                    sleep(segundos);
                }
                vm->pc++;
                break;
                
            case VM_PUSH:
                if (vm->sp < VM_STACK_SIZE - 1) {
                    vm->stack[++vm->sp] = vm->regs[R0];
                }
                vm->pc++;
                break;
                
            case VM_POP:
                if (vm->sp >= 0) {
                    vm->regs[R0] = vm->stack[vm->sp--];
                }
                vm->pc++;
                break;
                
            case VM_CMP:
                if (vm->sp >= 0) {
                    int top = vm->stack[vm->sp--];
                    vm->regs[R0] = top - vm->regs[R0];
                } else {
                    vm->regs[R0] = vm->regs[R0] - vm->regs[R1];
                }
                vm->pc++;
                break;
                
            case VM_EQ:
                if (vm->sp >= 0) {
                    int top = vm->stack[vm->sp--];
                    vm->regs[R0] = (top == vm->regs[R0]) ? 1 : 0;
                } else {
                    vm->regs[R0] = (vm->regs[R0] == vm->regs[R1]) ? 1 : 0;
                }
                vm->pc++;
                break;
                
            case VM_NE:
                if (vm->sp >= 0) {
                    int top = vm->stack[vm->sp--];
                    vm->regs[R0] = (top != vm->regs[R0]) ? 1 : 0;
                } else {
                    vm->regs[R0] = (vm->regs[R0] != vm->regs[R1]) ? 1 : 0;
                }
                vm->pc++;
                break;
                
            case VM_JMP:
                vm->pc = instr->arg1;
                break;
                
            case VM_JZ:
                if (vm->regs[R0] == 0) {
                    vm->pc = instr->arg1;
                } else {
                    vm->pc++;
                }
                break;
                
            case VM_JNZ:
                if (vm->regs[R0] != 0) {
                    vm->pc = instr->arg1;
                } else {
                    vm->pc++;
                }
                break;
                
            case VM_JLT:
                if (vm->regs[R0] < 0) {
                    vm->pc = instr->arg1;
                } else {
                    vm->pc++;
                }
                break;
                
            case VM_JGT:
                if (vm->regs[R0] > 0) {
                    vm->pc = instr->arg1;
                } else {
                    vm->pc++;
                }
                break;
                
            case VM_JLE:
                if (vm->regs[R0] <= 0) {
                    vm->pc = instr->arg1;
                } else {
                    vm->pc++;
                }
                break;
                
            case VM_JGE:
                if (vm->regs[R0] >= 0) {
                    vm->pc = instr->arg1;
                } else {
                    vm->pc++;
                }
                break;
                
            case VM_HALT:
                return;
                
            case VM_NOP:
                vm->pc++;
                break;
                
            default:
                fprintf(stderr, "Erro: Instrução desconhecida %d\n", instr->op);
                return;
        }
    }
    
    if (instruction_count >= max_instructions) {
        fprintf(stderr, "Erro: Limite de instruções excedido (possível loop infinito)\n");
    }
}

/* Carrega assembly de um arquivo .asm */
VM_State* vm_load_from_asm(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Erro: Não foi possível abrir o arquivo '%s'\n", filename);
        return NULL;
    }
    
    VM_State *vm = vm_create();
    char line[256];
    
    while (fgets(line, sizeof(line), f)) {
        /* Ignora comentários e linhas vazias */
        if (line[0] == ';' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        /* Remove espaços iniciais e finais */
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '\n') continue;
        
        /* Parse da linha: formato "   N: OPCODE args" */
        int addr;
        char opcode[32];
        int arg1 = 0;
        char label[64] = {0};
        
        if (sscanf(p, "%d: %31s", &addr, opcode) < 2) {
            continue;
        }
        
        /* Lê argumentos */
        char *args_start = strchr(p, ' ');
        int arg2 = 0;
        if (args_start) {
            args_start = strchr(args_start + 1, ' '); /* Pula o endereço */
            if (args_start) {
                args_start++; /* Pula o espaço após o opcode */
                while (*args_start == ' ' || *args_start == '\t') args_start++;
                
                if (*args_start == '\0' || *args_start == '\n') {
                    /* Sem argumentos */
                } else if (args_start[0] == ';') {
                    /* É um comentário com label (formato: " ; L1") */
                    args_start++;
                    while (*args_start == ' ' || *args_start == '\t') args_start++;
                    if (args_start[0] == 'L' || (args_start[0] >= 'A' && args_start[0] <= 'Z')) {
                        sscanf(args_start, "%63s", label);
                    }
                } else if (args_start[0] == 'L' || (args_start[0] >= 'A' && args_start[0] <= 'Z')) {
                    /* É um label direto */
                    sscanf(args_start, "%63s", label);
                } else if ((args_start[0] >= '0' && args_start[0] <= '9') || args_start[0] == '-') {
                    /* É um número - pode ter um ou dois argumentos */
                    if (sscanf(args_start, "%d , %d", &arg1, &arg2) == 2) {
                        /* Dois argumentos (ex: PISCAR 1, 1) */
                    } else {
                        /* Um argumento */
                        sscanf(args_start, "%d", &arg1);
                    }
                }
            }
        }
        
        /* Converte opcode string para enum */
        VM_Opcode op = VM_NOP;
        if (strcmp(opcode, "LOAD") == 0) op = VM_LOAD;
        else if (strcmp(opcode, "LOADM") == 0) op = VM_LOADM;
        else if (strcmp(opcode, "STORE") == 0) op = VM_STORE;
        else if (strcmp(opcode, "ADD") == 0) op = VM_ADD;
        else if (strcmp(opcode, "SUB") == 0) op = VM_SUB;
        else if (strcmp(opcode, "MUL") == 0) op = VM_MUL;
        else if (strcmp(opcode, "DIV") == 0) op = VM_DIV;
        else if (strcmp(opcode, "READ_SENSOR") == 0) op = VM_READ_SENSOR;
        else if (strcmp(opcode, "JMP") == 0) op = VM_JMP;
        else if (strcmp(opcode, "JZ") == 0) op = VM_JZ;
        else if (strcmp(opcode, "JNZ") == 0) op = VM_JNZ;
        else if (strcmp(opcode, "JLT") == 0) op = VM_JLT;
        else if (strcmp(opcode, "JGT") == 0) op = VM_JGT;
        else if (strcmp(opcode, "JLE") == 0) op = VM_JLE;
        else if (strcmp(opcode, "JGE") == 0) op = VM_JGE;
        else if (strcmp(opcode, "CMP") == 0) op = VM_CMP;
        else if (strcmp(opcode, "EQ") == 0) op = VM_EQ;
        else if (strcmp(opcode, "NE") == 0) op = VM_NE;
        else if (strcmp(opcode, "MUDAR") == 0) op = VM_MUDAR;
        else if (strcmp(opcode, "PISCAR") == 0) {
            op = VM_PISCAR;
            /* PISCAR usa arg1 (cor) e arg2 (número de vezes) */
        }
        else if (strcmp(opcode, "ESPERAR") == 0) {
            /* ESPERAR com argumento: gera LOAD + ESPERAR */
            if (arg1 != 0) {
                vm_add_instruction(vm, VM_LOAD, arg1, 0, NULL);
                op = VM_ESPERAR;
                arg1 = 0; /* ESPERAR usa R0 */
            } else {
                op = VM_ESPERAR;
            }
        }
        else if (strcmp(opcode, "PUSH") == 0) op = VM_PUSH;
        else if (strcmp(opcode, "POP") == 0) op = VM_POP;
        else if (strcmp(opcode, "HALT") == 0) op = VM_HALT;
        else if (strcmp(opcode, "NOP") == 0) op = VM_NOP;
        
        /* Para PISCAR, usa arg2; para outras instruções, arg2 = 0 */
        vm_add_instruction(vm, op, arg1, (op == VM_PISCAR) ? arg2 : 0, label[0] ? strdup(label) : NULL);
    }
    
    fclose(f);
    return vm;
}

void vm_print_code(VM_State *vm, FILE *out) {
    fprintf(out, "; Assembly gerado para VM Semaforos\n");
    
    /* Conta instruções reais (combinando LOAD+ESPERAR) */
    int real_count = 0;
    for (int i = 0; i < vm->code_size; i++) {
        if (i + 1 < vm->code_size && 
            vm->code[i].op == VM_LOAD && 
            vm->code[i + 1].op == VM_ESPERAR) {
            real_count++;
            i++; /* Pula o ESPERAR */
        } else {
            real_count++;
        }
    }
    fprintf(out, "; Total de instruções: %d\n\n", real_count);
    
    int line_num = 0;
    for (int i = 0; i < vm->code_size; i++) {
        VM_Instruction *instr = &vm->code[i];
        
        /* Otimização: se LOAD seguido de ESPERAR, combina em ESPERAR com valor */
        if (i + 1 < vm->code_size && 
            instr->op == VM_LOAD && 
            vm->code[i + 1].op == VM_ESPERAR) {
            /* Combina LOAD + ESPERAR em ESPERAR com valor */
            fprintf(out, "%4d: %-12s %d\n", line_num, "ESPERAR", instr->arg1);
            i++; /* Pula o ESPERAR seguinte */
            line_num++;
            continue;
        }
        
        fprintf(out, "%4d: %-12s", line_num, vm_opcode_name(instr->op));
        line_num++;
        
        switch (instr->op) {
            case VM_LOAD:
            case VM_LOADM:
            case VM_STORE:
            case VM_READ_SENSOR:
            case VM_PUSH:
            case VM_POP:
                fprintf(out, " %d", instr->arg1);
                break;
            case VM_MUDAR:
                fprintf(out, " %d", instr->arg1);
                break;
            case VM_ESPERAR:
                /* ESPERAR sem argumento usa R0 */
                fprintf(out, "");
                break;
            case VM_PISCAR:
                fprintf(out, " %d, %d", instr->arg1, instr->arg2);
                break;
            case VM_ADD:
            case VM_SUB:
            case VM_MUL:
            case VM_DIV:
            case VM_CMP:
            case VM_EQ:
            case VM_NE:
                /* Operações binárias não têm argumentos impressos */
                fprintf(out, "");
                break;
            case VM_JMP:
            case VM_JZ:
            case VM_JNZ:
            case VM_JLT:
            case VM_JGT:
            case VM_JLE:
            case VM_JGE:
                if (instr->label) {
                    fprintf(out, " %s", instr->label);
                } else {
                    fprintf(out, " %d", instr->arg1);
                }
                break;
            case VM_NOP:
                if (instr->label) {
                    fprintf(out, " ; %s", instr->label);
                }
                break;
            case VM_HALT:
                fprintf(out, "");
                break;
            default:
                break;
        }
        fprintf(out, "\n");
    }
}
