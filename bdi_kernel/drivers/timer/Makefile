# Timer Subsystem Makefile

CC := gcc
CFLAGS := -Wall -Wextra -std=gnu11 -O2 -g -I../../include
LDFLAGS := -lpthread

SRCS := hpet.c rtc.c clocksource.c
OBJS := $(SRCS:.c=.o)
TARGET := libtimer.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
