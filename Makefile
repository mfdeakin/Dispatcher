
CC=gcc
CFLAGS=-g -Wall -std=gnu99
OBJCOPY=objcopy
OLDSYMNAME=_binary_usage_txt_start
NEWSYMNAME=semusage
OBJCOPYFLAGS=-I binary -O elf64-x86-64 -B i386 --redefine-sym $(OLDSYMNAME)=$(NEWSYMNAME)

all: startup

startup: startup.o libipc.so
	$(CC) $(CFLAGS) -L. -lipc -o $@ $<

libipc.so: libipc.c myipc.h
	$(CC) $(CFLAGS) -c -Werror -fpic -o libipc.o libipc.c
	$(CC) -shared -o libipc.so libipc.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
