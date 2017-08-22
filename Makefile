SHELL = /bin/sh

CFLAGS = -O2 -g -Wall -Wextra -pedantic -std=c11 -I src

BIN = ./bin
OBJ = ./obj
WEBOBJ= ./webobj
SRC = ./src
WSM = ./bin/wsm

all: directories parse print_tokens

directories:
	mkdir -p $(BIN) $(OBJ) $(WEBOBJ) $(WSM)

parse: $(SRC)/drivers/parse.c lex.o parse.o token.o value.o ASTNode.o mem.o
	clang $(CFLAGS) -o $(BIN)/parse $(SRC)/drivers/parse.c \
		$(OBJ)/lex.o $(OBJ)/parse.o $(OBJ)/token.o \
		$(OBJ)/value.o $(OBJ)/ASTNode.o $(OBJ)/mem.o
	emcc -s WASM=1 $(CFLAGS) -o $(WSM)/parse.html $(SRC)/drivers/parse.c \
		$(WEBOBJ)/lex.o $(WEBOBJ)/parse.o $(WEBOBJ)/token.o \
		$(WEBOBJ)/value.o $(WEBOBJ)/ASTNode.o $(WEBOBJ)/mem.o

print_tokens: $(SRC)/drivers/print_tokens.c \
		lex.o print.o token.o mem.o
	clang $(CFLAGS) -o $(BIN)/print_tokens $(SRC)/drivers/print_tokens.c \
		$(OBJ)/lex.o $(OBJ)/print.o $(OBJ)/token.o \
		$(OBJ)/mem.o

clean:
	rm -rf $(OBJ) $(BIN)

lex.o: $(SRC)/lex/lex.c $(SRC)/token/token.h
	clang -c $(CFLAGS) -o $(OBJ)/lex.o $(SRC)/lex/lex.c
	emcc  -c $(CFLAGS) -o $(WEBOBJ)/lex.o $(SRC)/lex/lex.c

print.o: $(SRC)/parse/print.c $(SRC)/token/token.h
	clang -c $(CFLAGS) -o $(OBJ)/print.o $(SRC)/parse/print.c
	emcc -c $(CFLAGS) -o $(WEBOBJ)/print.o $(SRC)/parse/print.c

parse.o: $(SRC)/parse/parse.c $(SRC)/token/token.h
	clang -c $(CFLAGS) -o $(OBJ)/parse.o $(SRC)/parse/parse.c
	emcc -c $(CFLAGS) -o $(WEBOBJ)/parse.o $(SRC)/parse/parse.c

token.o: $(SRC)/token/token.c $(SRC)/token/token.h
	clang -c $(CFLAGS) -o $(OBJ)/token.o $(SRC)/token/token.c
	emcc -c $(CFLAGS) -o $(WEBOBJ)/token.o $(SRC)/token/token.c

value.o: $(SRC)/value/value.c $(SRC)/value/value.h
	clang -c $(CFLAGS) -o $(OBJ)/value.o $(SRC)/value/value.c
	emcc -c $(CFLAGS) -o $(WEBOBJ)/value.o $(SRC)/value/value.c

ASTNode.o: $(SRC)/parse/ASTNode.c $(SRC)/parse/ASTNode.h
	clang -c $(CFLAGS) -o $(OBJ)/ASTNode.o $(SRC)/parse/ASTNode.c
	emcc -c $(CFLAGS) -o $(WEBOBJ)/ASTNode.o $(SRC)/parse/ASTNode.c

mem.o: $(SRC)/mem/mem.c $(SRC)/mem/mem.h
	clang -c $(CFLAGS) -o $(OBJ)/mem.o $(SRC)/mem/mem.c
	emcc -c $(CFLAGS) -o $(WEBOBJ)/mem.o $(SRC)/mem/mem.c
