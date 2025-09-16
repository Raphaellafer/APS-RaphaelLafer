#  Semaforos

##  Objetivo
A linguagem **Semaforos** foi criada para simular e programar o comportamento de um semáforo virtual em uma Máquina de Estados.  
O propósito é oferecer uma forma de **programar sinais de trânsito** , utilizando conceitos de linguagens de programação como **variáveis, condicionais e laços**.  

Enquanto outras linguagens controlam robôs, microondas ou carros, o **Semaforos** foca em um problema urbano clássico:  
como organizar o tempo de verde, amarelo e vermelho, respeitando o fluxo de carros e dependendo do horario do dia.

---

##  Especificação em EBNF

```ebnf
Program     = { Statement } ;

Statement   = Assignment | IfStmt | WhileStmt | Command ;

Assignment  = Identifier "=" Expression ";" ;

IfStmt      = "if" "(" Condition ")" "{" { Statement } "}" 
            [ "else" "{" { Statement } "}" ] ;

WhileStmt   = "while" "(" Condition ")" "{" { Statement } "}" ;

Command     = LightCmd ";" | SensorCmd ";" | TimerCmd ";" ;

LightCmd    = "mudar" "(" Color ")"               
            | "piscar" "(" Color "," Expression ")" ;

SensorCmd   = "ler" "(" Sensor ")" "->" Identifier ;

TimerCmd    = "esperar" "(" Expression ")" ;

Expression  = Term { ("+" | "-" | "*" | "/") Term } ;
Term        = Number | Identifier | "(" Expression ")" ;

Condition   = Expression RelOp Expression ;
RelOp       = "==" | "!=" | ">" | "<" | ">=" | "<=" ;

Color       = "verde" | "amarelo" | "vermelho" ;
Sensor      = "horario" | "duracao" | "fluxo" ;

Identifier  = Letter { Letter | Digit | "_" } ;
Number      = Digit { Digit } ;

Letter      = "a" | ... | "z" | "A" | ... | "Z" ;
Digit       = "0" | ... | "9" ;

## Unidades de medida

-horario → representa a hora do dia, variando de 0 a 23 (formato 24 horas).
Exemplo: 0 = meia-noite, 12 = meio-dia, 23 = 23h. Esse valor é atualizado automaticamente pela VM, simulando um relógio.

-duracao → representa o tempo em segundos que o semáforo permanece em uma cor específica. Quando a cor muda, esse valor reinicia em 0.

-fluxo → representa a quantidade de carros detectados em determinada via. É sempre um número inteiro.

-esperar(x) → comando que faz o programa pausar por x segundos antes de continuar a execução.


## Sensores disponíveis

horario → retorna a hora do dia (inteiro de 0 a 23).

0 = meia-noite (00h)

6 = 6h da manhã

12 = meio-dia

20 = 20h (8 da noite)

23 = 23h (11 da noite)

Após 23, o valor volta para 0.

duracao → tempo em segundos que o semáforo está na cor atual.

Exemplo: se o sinal ficou verde por 12 segundos, ler(duracao) retorna 12.

fluxo → quantidade de carros detectados no cruzamento.

Exemplo: ler(fluxo) -> f; pode retornar 15 (15 carros na via).

##  Exemplo de um programa

ler(horario) -> h;

if (h >= 6 && h < 20) {
    // Dia: das 06h até 19h59
    mudar(verde);
    esperar(25);   // 25 segundos
    mudar(amarelo);
    esperar(4);    // 4 segundos
    mudar(vermelho);
    esperar(18);   // 18 segundos
} else {
    // Noite: das 20h até 05h59
    mudar(verde);
    esperar(25);
    mudar(amarelo);
    esperar(4);
    mudar(vermelho);
    esperar(14);
}

O código acima le o horario em que o dia se encontra, se está entre as 6h00 e 19h59, o semaforo vermelho dura mais (18 segundos), se o horário for entre 20h00 e 5h59, o semaforo vermelho dura menos (14 segundos) 
