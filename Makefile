CC = gcc
CFLAGS = -Wall -Wextra -g
FLEX = flex
BISON = bison

all: semaforos

semaforos: parser.tab.o lex.yy.o main.o
	$(CC) -o $@ $^ -lfl

parser.tab.c parser.tab.h: parser.y
	$(BISON) -d parser.y

lex.yy.c: lexer.l parser.tab.h
	$(FLEX) lexer.l

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f semaforos *.o parser.tab.c parser.tab.h lex.yy.c

.PHONY: all clean