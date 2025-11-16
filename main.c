#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vm.h"

extern int yyparse(void);
extern int yylineno;
extern FILE *yyin;
extern void yyrestart(FILE *);

enum Cor { VERDE_C, AMARELO_C, VERMELHO_C };
extern enum Cor cor_atual;

/* Variáveis globais para acessar do parser */
extern VM_State *vm;
extern int executar_durante_compilacao; /* Declarado em parser.y */

void mostrar_ajuda() {
    printf("\n=== COMANDOS DISPONÍVEIS ===\n");
    printf("mudar(verde|amarelo|vermelho);\n");
    printf("esperar(segundos);\n");
    printf("ler(horario|duracao|fluxo) -> variavel;\n");
    printf("variavel = expressao;\n");
    printf("if (condicao) { ... } else { ... }\n");
    printf("while (condicao) { ... }\n");
    printf("fim - encerra\n");
    printf("executar - programa multi-linha\n");
    printf("ajuda - mostra ajuda\n");
    printf("=============================\n");
}

void mostrar_cor_atual() {
    const char *nome_cor;
    switch(cor_atual) {
        case VERDE_C: nome_cor = "VERDE"; break;
        case AMARELO_C: nome_cor = "AMARELO"; break;
        case VERMELHO_C: nome_cor = "VERMELHO"; break;
        default: nome_cor = "DESCONHECIDA"; break;
    }
    printf("[Semaforo: %s] > ", nome_cor);
}

void executar_programa_completo() {
    printf("\n=== MODO PROGRAMA COMPLETO ===\n");
    printf("Digite seu programa linha por linha.\n");
    printf("Termine com 'FIM' em uma linha separada:\n\n");
    
    char *programa = malloc(10000);
    programa[0] = '\0';
    char linha[256];
    int linhas = 0;
    
    while (1) {
        printf("%3d: ", linhas + 1);
        if (fgets(linha, sizeof(linha), stdin) == NULL) {
            break;
        }
        
        linha[strcspn(linha, "\n")] = 0;
        
        if (strcmp(linha, "FIM") == 0 || strcmp(linha, "fim") == 0) {
            break;
        }
        
        /* Adiciona ponto e vírgula se necessário */
        int len = strlen(linha);
        if (len > 0 && linha[len-1] != ';' && linha[len-1] != '}') {
            strcat(programa, linha);
            strcat(programa, ";");
        } else {
            strcat(programa, linha);
        }
        strcat(programa, "\n");
        linhas++;
    }
    
    if (linhas > 0) {
        printf("\nExecutando programa...\n");
        
        /* Ativa execução durante compilação */
        executar_durante_compilacao = 1;
        
        FILE *temp = fmemopen(programa, strlen(programa), "r");
        if (temp) {
            yyin = temp;
            yylineno = 1;
            yyparse();
            fclose(temp);
        }
        
        /* Desativa execução após compilação */
        executar_durante_compilacao = 0;
        
        printf("Programa finalizado.\n\n");
    }
    
    free(programa);
}

void carregar_arquivo(const char *nome_arquivo) {
    /* Verifica se é um arquivo .asm */
    int len = strlen(nome_arquivo);
    if (len >= 4 && strcmp(nome_arquivo + len - 4, ".asm") == 0) {
        printf("\nExecutando programa do arquivo '%s' na VM...\n", nome_arquivo);
        
        VM_State *vm_exec = vm_load_from_asm(nome_arquivo);
        if (vm_exec) {
            vm_execute(vm_exec);
            vm_destroy(vm_exec);
            printf("Programa executado com sucesso!\n\n");
        } else {
            printf("Erro ao carregar arquivo .asm\n\n");
        }
        return;
    }
    
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        printf("Erro: Não foi possível abrir o arquivo '%s'\n", nome_arquivo);
        return;
    }
    
    printf("\nCompilando e executando programa do arquivo '%s'...\n", nome_arquivo);
    
    /* Ativa execução durante compilação para arquivos .sema */
    executar_durante_compilacao = 1;
    
    yyin = arquivo;
    yylineno = 1;
    int resultado = yyparse();
    
    fclose(arquivo);
    
    /* Desativa execução após compilação */
    executar_durante_compilacao = 0;
    
    if (resultado == 0) {
        /* Gera arquivo .asm */
        char asm_filename[512];
        strncpy(asm_filename, nome_arquivo, sizeof(asm_filename) - 1);
        asm_filename[sizeof(asm_filename) - 1] = '\0';
        len = strlen(asm_filename);
        if (len >= 5 && strcmp(asm_filename + len - 5, ".sema") == 0) {
            strcpy(asm_filename + len - 5, ".asm");
            FILE *asm_file = fopen(asm_filename, "w");
            if (asm_file && vm) {
                vm_print_code(vm, asm_file);
                fclose(asm_file);
                printf("\nArquivo .asm gerado: %s\n", asm_filename);
                printf("Para executar na VM, use: carregar %s\n\n", asm_filename);
            } else {
                printf("Erro ao gerar arquivo .asm\n\n");
            }
        } else {
            printf("Programa executado com sucesso!\n\n");
        }
    } else {
        printf("Programa finalizado com erros.\n\n");
    }
}

int main() {
    srand(time(NULL));
    cor_atual = VERDE_C;
    
    char entrada[256];
    int continuar = 1;
    
    printf("=== Interpretador Semaforos ===\n");
    printf("Digite 'ajuda' para ver os comandos\n\n");
    
    while(continuar) {
        mostrar_cor_atual();
        
        if (fgets(entrada, sizeof(entrada), stdin) == NULL) {
            break;
        }
        
        entrada[strcspn(entrada, "\n")] = 0;
        
        if (strcmp(entrada, "fim") == 0 || strcmp(entrada, "FIM") == 0) {
            printf("Encerrando programa...\n");
            break;
        }
        
        if (strcmp(entrada, "ajuda") == 0 || strcmp(entrada, "help") == 0) {
            mostrar_ajuda();
            continue;
        }
        
        if (strcmp(entrada, "executar") == 0) {
            executar_programa_completo();
            continue;
        }
        
        if (strncmp(entrada, "carregar ", 9) == 0) {
            carregar_arquivo(entrada + 9);
            continue;
        }
        
        if (strlen(entrada) == 0) {
            continue;
        }
        

        char linha[300];
        strcpy(linha, entrada);
        if (linha[strlen(linha)-1] != ';' && linha[strlen(linha)-1] != '}') {
            strcat(linha, ";");
        }
        
        FILE *temp = fmemopen(linha, strlen(linha), "r");
        if (temp) {
            yyin = temp;
            yylineno = 1;
            
            int resultado = yyparse();
            
            yyrestart(NULL);
            
            fclose(temp);
            
            if (resultado != 0) {
                printf("Erro no comando. Tente novamente.\n");
            }
        }
    }
    
    printf("Programa finalizado.\n");
    return 0;
}