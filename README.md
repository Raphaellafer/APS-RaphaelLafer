make # Semaforos - Linguagem de Programação para Controle de Semáforos

## Objetivo
A linguagem Semaforos foi criada para simular e programar o comportamento de um semáforo virtual em uma Máquina de Estados. O propósito é oferecer uma forma de programar sinais de trânsito, utilizando conceitos de linguagens de programação como variáveis, condicionais e laços.

Enquanto outras linguagens controlam robôs, microondas ou carros, o Semaforos foca em um problema urbano clássico: como organizar o tempo de verde, amarelo e vermelho, respeitando o fluxo de carros e dependendo do horário do dia.

## Especificação da Linguagem

### EBNF
```
Program = { Statement } ;

Statement = Assignment | IfStmt | WhileStmt | Command ;

Assignment = Identifier "=" Expression ";" ;

IfStmt = "if" "(" Condition ")" "{" { Statement } "}"
[ "else" "{" { Statement } "}" ] ;

WhileStmt = "while" "(" Condition ")" "{" { Statement } "}" ;

Command = LightCmd ";" | SensorCmd ";" | TimerCmd ";" ;

LightCmd = "mudar" "(" Color ")"
| "piscar" "(" Color "," Expression ")" ;

SensorCmd = "ler" "(" Sensor ")" "->" Identifier ;

TimerCmd = "esperar" "(" Expression ")" ;

Expression = Term { ("+" | "-" | "*" | "/") Term } ;
Term = Number | Identifier | "(" Expression ")" ;

Condition = LogicOr ;
LogicOr = LogicAnd { "||" LogicAnd } ;
LogicAnd = RelCondition { "&&" RelCondition } ;
RelCondition = Expression RelOp Expression ;

RelOp = "==" | "!=" | ">" | "<" | ">=" | "<=" ;

Color = "verde" | "amarelo" | "vermelho" ;
Sensor = "horario" | "duracao" | "fluxo" ;

Identifier = Letter { Letter | Digit | "_" } ;
Number = Digit { Digit } ;
```


### Sensores e Unidades de Medida

- **horario** → representa a hora do dia (0-23)
- **duracao** → tempo em segundos na cor atual
- **fluxo** → quantidade de carros detectados
- **esperar(x)** → pausa por x segundos

### Comandos Disponíveis

#### Controle de Semáforo
```
mudar(verde);       // Muda para verde
mudar(amarelo);     // Muda para amarelo  
mudar(vermelho);    // Muda para vermelho
piscar(verde, 5);   // Pisca verde 5 vezes
```
Sensores
```
ler(horario) -> h;  // Lê hora atual para variável h
ler(fluxo) -> f;    // Lê fluxo de carros para f
ler(duracao) -> d;  // Lê duração atual para d
```
Temporização
```
esperar(10);        // Espera 10 segundos
```
Estruturas de Controle
```
if (condição) {
    // comandos
} else {
    // comandos
}

while (condição) {
    // comandos
}
```
Exemplo
```
mudar(verde)
esperar(20)
mudar(amarelo)
esperar(5)
mudar(vermelho)
fim

```
Implementação
Arquivos do Projeto
parser.y - Analisador sintático Bison

lexer.l - Analisador léxico Flex

main.c - Programa principal e modo interativo

Makefile - Sistema de compilação

Compilação
```
make clean
make
./semaforos
```
Modos de Uso
Modo Interativo
```
[Semaforo: VERDE] > mudar(amarelo);
[Semaforo: AMARELO] > esperar(5);
```
Modo Programa Completo
```
[Semaforo: VERDE] > executar
# Digite o programa linha por linha
# Termine com 'FIM'
```
Carregar Arquivo
```
[Semaforo: VERDE] > carregar programa.sema
```
