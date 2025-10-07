# Makefile for ML CodeGen

CC = gcc
CFLAGS = -std=gnu2x -Wall -Wextra -Wpedantic -O2 -march=native -I../..
LDFLAGS = -lm

# Source files
ML_CODEGEN_SRCS = ml_codegen.c
ML_CODEGEN_OBJS = $(ML_CODEGEN_SRCS:.c=.o)

# Library
LIBML_CODEGEN = libml_codegen.a

.PHONY: all clean

all: $(LIBML_CODEGEN)

$(LIBML_CODEGEN): $(ML_CODEGEN_OBJS)
	ar rcs $@ $^
	@echo "Built ML CodeGen library: $(LIBML_CODEGEN)"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(ML_CODEGEN_OBJS) $(LIBML_CODEGEN)
