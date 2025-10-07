# IRQ Subsystem Makefile

CC := gcc
CFLAGS := -Wall -Wextra -std=gnu11 -O2 -g -I../../include
LDFLAGS := -lpthread

SRCS := ioapic.c
OBJS := $(SRCS:.c=.o)
TARGET := libirq.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
