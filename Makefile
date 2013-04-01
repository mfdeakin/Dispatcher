
CC=gcc
CFLAGS=-g -Wall -std=gnu99
OBJCOPY=objcopy
OLDSYMNAME=_binary_usage_txt_start
NEWSYMNAME=semusage
OBJCOPYFLAGS=-I binary -O elf64-x86-64 -B i386 --redefine-sym $(OLDSYMNAME)=$(NEWSYMNAME)

all: startup user

startup: startup.o libmystuff.so
	$(CC) $(CFLAGS) -L. -lmystuff -o $@ $<

user: user.o libmystuff.so
	$(CC) $(CFLAGS) -L. -lmystuff -o $@ $<

libmystuff.so: libipc.c myipc.h
	$(CC) $(CFLAGS) -c -Werror -fpic -o libipc.o libipc.c
	$(CC) -shared -o libmystuff.so libipc.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
