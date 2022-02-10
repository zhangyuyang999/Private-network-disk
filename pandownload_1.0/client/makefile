SRCS:=$(wildcard *.c)
TARGS:=$(SRCS:%.c=%)
CC:=gcc
all:$(TARGS)

%:%.c
		$(CC) $< -o $@ -lpthread
clean:
		rm -rf $(TARGS)

