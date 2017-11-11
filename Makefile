BUILD = $(shell find $(SRC)/* -maxdepth 0 -type d)

export BIN = $(PWD)/bin
export DEPS = $(PWD)/obj/deps
export HEADERS = $(PWD)/lib/headers
export LIBOBJ = $(PWD)/lib/obj
export OBJ = $(PWD)/obj
export SHELL = /bin/sh
export SRC = $(PWD)/src
export WEBOBJ= $(PWD)/webobj
export WSM = $(PWD)/bin/wsm

export CFLAGS = -O2 -g -Wall -Wextra -pedantic -std=c11 -I $(SRC) -I $(HEADERS)
export VPATH = $(OBJ):$(WEBOBJ):$(BIN)

.PHONY: directories $(BUILD) clean
all: directories executables $(BUILD)

$(BUILD):
	$(MAKE) -C $@

clean:
	rm -rf $(OBJ) $(BIN)
	find $(SRC) -type f -name "*.d" | xargs rm

directories:
	mkdir -p $(BIN) $(OBJ) $(WEBOBJ) $(WSM) $(LIB) $(LIBOBJ) $(DEPS)

executables: parse print

parse: $(BUILD)
	$(CC) $(CFLAGS) -o $(BIN)/parse \
		$(OBJ)/parse_driver.o $(OBJ)/parse.o $(OBJ)/ASTNode.o $(OBJ)/mem.o\
		$(OBJ)/value.o $(OBJ)/token.o $(OBJ)/lex.o $(OBJ)/symtable.o \
		$(LIBOBJ)/xxhash.o

print: $(BUILD)
	$(CC) $(CFLAGS) -o $(BIN)/print \
		$(OBJ)/print_driver.o $(OBJ)/token.o $(OBJ)/lex.o $(OBJ)/mem.o
