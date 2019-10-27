.PHONY: clean

CC = gcc
CFLAGS = -ansi -pedantic -Wall -Wextra -g -D_XOPEN_SOURCE=60000

all: mysh myps myls

mysh:   srcMysh/main.c srcMysh/interpreter.o srcMysh/redirection.o srcMysh/message.o srcMysh/background.o srcMysh/variables.o
	$(CC) -o $@ $^

myps:	srcMyps/myps.c
	$(CC) $(CFLAGS) -o $@ $^

myls:	srcMyls/myls.c
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f srcMysh/*.o mysh myps myls
