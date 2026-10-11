
#ifndef PHASE1_H
#define PHASE1_H

#include <inttypes.h>   // PRIx64, SCNu64... pour afficher / lire des uint64_t proprement
#include "../rsa_common_header.h"

/* ---------- rsa_tools.c (fourni) ---------- */
void     erreur(char *msg);
uint64_t random_uint(uint64_t min, uint64_t max);
int      premier(uint64_t n);
int      decompose(uint64_t facteur[], uint64_t n);
uint64_t puissance(uint64_t a, uint64_t e);
uint64_t puissance_mod_n(uint64_t a, uint64_t e, uint64_t n);
uint64_t genereUint(uint64_t max, int *cpt);
int      rabin(uint64_t a, uint64_t n);
int64_t  genereUintRabin(uint64_t max, int *cpt);
uint64_t pgcdFast(uint64_t a, uint64_t b);
void     genKeysRabin(rsaKey_t *pubKey, rsaKey_t *privKey, uint64_t max_prime);
void     inputKey(uint64_t E, uint64_t N, rsaKey_t *key);
void     verifRabin(uint64_t max, int iterations);

/* ---------- bezout.c (fourni) ---------- */
int64_t bezout(uint64_t a, uint64_t b, int64_t *u, int64_t *v);
int64_t bezoutRSA(uint64_t a, uint64_t b, int64_t *u, int64_t *v);

/* ---------- log.c (Nada) ---------- */
void init_log(void);    // à appeler au début de TOUT main (sinon logfp est NULL et ça plante)
void close_log(void);

/* ---------- keys_io.c (Nada) ---------- */
void print_key_hex(const char *label, const rsaKey_t *key);
void print_keypair_hex(const keyPair_t *kp);
void gen_keypair(keyPair_t *kp, uint64_t max_prime);
int  save_keypair(const char *path, const keyPair_t *kp);   // 0 = ok, -1 = erreur
int  load_keypair(const char *path, keyPair_t *kp);         // 0 = ok, -1 = erreur

#endif
