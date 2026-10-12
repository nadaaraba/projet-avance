CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: test_phase1

test_phase1: main_phase1.o phase1.o rsa_tools.c bezout.c other_base64.c
	$(CC) $(CFLAGS) -o test_phase1 main_phase1.o phase1.o rsa_tools.c bezout.c other_base64.c

main_phase1.o: main_phase1.c phase1.h
	$(CC) $(CFLAGS) -c main_phase1.c

phase1.o: phase1.c phase1.h
	$(CC) $(CFLAGS) -c phase1.c

clean:
	rm -f *.o test_phase1 msg_clair.txt msg_chiffre.txt msg_dechiffre.txt msg_chiffre.b64 msg_recup.bin keys_test.txt