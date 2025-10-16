#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern int yyparse(void);
extern int yylineno;
extern FILE *yyin;

enum Cor { VERDE_C, AMARELO_C, VERMELHO_C };
extern enum Cor cor_atual;

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
        
        strcat(programa, linha);
        strcat(programa, "\n");
        linhas++;
    }
    
    if (linhas > 0) {
        printf("\nExecutando programa...\n");
        
        FILE *temp = fmemopen(programa, strlen(programa), "r");
        if (temp) {
            yyin = temp;
            yylineno = 1;
            yyparse();
            fclose(temp);
        }
        printf("Programa finalizado.\n\n");
    }
    
    free(programa);
}

void carregar_arquivo(const char *nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        printf("Erro: Não foi possível abrir o arquivo '%s'\n", nome_arquivo);
        return;
    }
    
    printf("\nExecutando programa do arquivo '%s'...\n", nome_arquivo);
    
    yyin = arquivo;
    yylineno = 1;
    yyparse();
    
    fclose(arquivo);
    printf("Programa finalizado.\n\n");
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
            yyparse();
            fclose(temp);
        }
    }
    
    printf("Programa finalizado.\n");
    return 0;
}