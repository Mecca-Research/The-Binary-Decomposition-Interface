# Makefile for ML VM Operations

CC = gcc
CFLAGS = -std=gnu2x -Wall -Wextra -Wpedantic -O2 -march=native -I../..
LDFLAGS = -lm

# Source files
ML_OPS_SRCS = ml_ops.c
ML_OPS_OBJS = $(ML_OPS_SRCS:.c=.o)

# Library
LIBML_OPS = libml_ops.a

.PHONY: all clean

all: $(LIBML_OPS)

$(LIBML_OPS): $(ML_OPS_OBJS)
	ar rcs $@ $^
	@echo "Built ML VM Ops library: $(LIBML_OPS)"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(ML_OPS_OBJS) $(LIBML_OPS)
