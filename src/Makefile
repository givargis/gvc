# Copyright (c) Tony Givargis, 2024-2026

CC      = gcc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Werror -Wfatal-errors -fPIC -O3
LDFLAGS = -L/usr/local/cuda-13/lib64 -lm -lpthread -lcudart
DEST    = gvc

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

MODULES = \
	cuda/libcuda.a \
	lang/liblang.a \
	util/libutil.a

all: modules $(OBJS)
	@echo "[LN] $(DEST)"
	@$(CC) -o $(DEST) $(OBJS) $(MODULES) $(LDFLAGS)

%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $<
	@$(CC) $(CFLAGS) -MM $< > $*.d

modules:
	@make -C cuda
	@make -C lang
	@make -C util

clean:
	@rm -f $(DEST) *.o *.d *~ *#
	@make -C cuda clean
	@make -C lang clean
	@make -C util clean

-include $(OBJS:.o=.d)
