include ../pattern.mk

all: ASTNode.o

sources = ASTNode.c
include $(sources:.c=.d)

ASTNode.o: ASTNode.c ASTNode.d

ASTNode.d: ASTNode.c
