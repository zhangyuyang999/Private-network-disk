SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))
ELF:= main	
CC:=gcc
CFLAGS:=-g -Wall
$(ELF):$(OBJS)
	gcc $^ -o $@ -I /usr/include/mysql/ -lmysqlclient -lcrypt

clean:
	rm -rf $(OBJS)  $(ELF)
