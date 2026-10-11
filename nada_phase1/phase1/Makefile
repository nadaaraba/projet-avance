# Makefile de la phase 1 (Nada). Les autres ajoutent leurs cibles ICI (ne pas créer un 2e Makefile).
CC      = gcc
CFLAGS  = -Wall -Wextra -std=gnu11 -g
COMMUN  = rsa_tools.o bezout.o keys_io.o log.o

all: test_cles test_maxprime

test_cles: test_cles.o $(COMMUN)
	$(CC) $(CFLAGS) -o $@ $^

test_maxprime: test_maxprime.o $(COMMUN)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c phase1.h ../rsa_common_header.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o test_cles test_maxprime diypg.log cles_test.key

.PHONY: all clean
