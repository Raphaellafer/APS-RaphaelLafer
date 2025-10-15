BISON = bison
FLEX = flex
CC = gcc

PARSER = parser
LEXER = lexer
MAIN = main

EXEC = semaforos

all: $(EXEC)

$(EXEC): $(PARSER).tab.c lex.yy.c $(MAIN).c
	$(CC) -o $(EXEC) $(MAIN).c $(PARSER).tab.c lex.yy.c -lfl

$(PARSER).tab.c $(PARSER).tab.h: $(PARSER).y
	$(BISON) -d $(PARSER).y

lex.yy.c: $(LEXER).l $(PARSER).tab.h
	$(FLEX) $(LEXER).l

clean:
	rm -f $(EXEC) $(PARSER).tab.c $(PARSER).tab.h lex.yy.c
