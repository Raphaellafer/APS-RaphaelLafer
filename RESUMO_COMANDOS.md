# Resumo dos Comandos de Teste

## Comandos Disponíveis

### 1. Compilar e Executar (COM Assembly)
```bash
./semaforos
carregar exemplos/nome_arquivo.sema
fim
```
**O que faz:**
- Compila o arquivo `.sema`
- Gera o arquivo `.asm` 
- Executa na VM

### 2. Executar Assembly Diretamente (SEM recompilar)
```bash
./semaforos
executar_asm exemplos/nome_arquivo.asm
fim
```
**O que faz:**
- Carrega o arquivo `.asm` já existente
- Executa na VM (sem compilar)


## Arquivos Gerados

Todos os arquivos `.asm` são gerados automaticamente na pasta `exemplos/` quando você usa o comando para carregar o sema.
