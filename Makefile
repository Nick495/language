SHELL = /bin/sh

CFLAGS = -O2 -Wall -Wextra -pedantic -std=c11

BIN = ./bin
OBJ = ./obj
SRC = ./src

all: directories parse print_tokens

directories:
	mkdir -p $(BIN) $(OBJ)

parse: $(SRC)/drivers/parse.c lex.o parse.o token.o string.o
	clang $(CFLAGS) -o $(BIN)/parse $(SRC)/drivers/parse.c \
		$(OBJ)/lex.o $(OBJ)/parse.o $(OBJ)/token.o $(OBJ)/string.o

print_tokens: $(SRC)/drivers/print_tokens.c lex.o print.o token.o string.o
	clang $(CFLAGS) -o $(BIN)/print_tokens $(SRC)/drivers/print_tokens.c \
		$(OBJ)/lex.o $(OBJ)/print.o $(OBJ)/token.o $(OBJ)/string.o

clean:
	rm -rf $(OBJ) $(BIN)

lex.o: $(SRC)/lex/lex.c $(SRC)/token/token.h
	clang -c $(CFLAGS) -o $(OBJ)/lex.o $(SRC)/lex/lex.c

print.o: $(SRC)/parse/print.c $(SRC)/token/token.h
	clang -c $(CFLAGS) -o $(OBJ)/print.o $(SRC)/parse/print.c

parse.o: $(SRC)/parse/parse.c $(SRC)/token/token.h
	clang -c $(CFLAGS) -o $(OBJ)/parse.o $(SRC)/parse/parse.c

string.o: $(SRC)/string/string.c $(SRC)/string/string.h
	clang -c $(CFLAGS) -o $(OBJ)/string.o $(SRC)/string/string.c

token.o: $(SRC)/token/token.c $(SRC)/token/token.h
	clang -c $(CFLAGS) -o $(OBJ)/token.o $(SRC)/token/token.c
