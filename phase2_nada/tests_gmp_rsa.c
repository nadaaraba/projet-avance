/// \file tests_gmp_rsa.c
/// \author Nada ARABA
/// \brief Tests de la phase 2 (partie Nada) : puissance_mod_n_gmp, clefs > 2^32, chiffrement d'un bloc.
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "gmp_rsa.h"
#include "../phase1_nada/phase1/phase1.h"   /* puissance_mod_n (version uint64 de la phase 1) */

static int echecs = 0;
#define VERIF(cond) do { if (!(cond)) { printf("ECHEC ligne %d : %s\n", __LINE__, #cond); echecs++; } } while (0)

static void test_puissance(void) {
    /* Pour n <= 2^32, la version GMP doit donner exactement la meme chose que la phase 1. */
    for (uint64_t i = 0; i < 20000; i++) {
        uint64_t n = 2 + random_uint(0, 4294967294ULL);
        uint64_t a = random_uint(0, n - 1), e = random_uint(0, 100000);
        VERIF(puissance_mod_n_gmp_u64(a, e, n) == puissance_mod_n(a, e, n));
    }
    VERIF(puissance_mod_n_gmp_u64(5, 3, 0) == 0);
    VERIF(puissance_mod_n_gmp_u64(0, 0, 7) == 1);          /* 0^0 = 1 pour mpz_powm */
    VERIF(puissance_mod_n_gmp_u64(UINT64_MAX, 2, UINT64_MAX) == 0);
    VERIF(puissance_mod_n_gmp_u64(UINT64_MAX - 1, 2, UINT64_MAX) == 1);   /* (-1)^2 mod (2^64-1) */
    VERIF(puissance_mod_n_gmp_u64(2, 64, UINT64_MAX) == 1);               /* 2^64 = 1 mod (2^64 - 1) */
}

static void test_phase1_deborde(void) {
    /* Avec N proche de 2^56, a*a deborde en uint64 : la phase 1 se trompe, la version GMP non. */
    rsaKey_t pub, priv;
    int erreurs_phase1 = 0;
    VERIF(gen_keys_gmp(&pub, &priv, 28) == RSA_SUCCES);
    for (int i = 0; i < 2000; i++) {
        uint64_t m = random_uint(1, pub.N - 1);
        uint64_t c = puissance_mod_n_gmp_u64(m, pub.E, pub.N);
        if (puissance_mod_n(m, pub.E, pub.N) != c) erreurs_phase1++;
        VERIF(puissance_mod_n_gmp_u64(c, priv.E, priv.N) == m);   /* GMP : aller-retour toujours correct */
    }
    printf("  (information) N = %" PRIu64 " : la phase 1 (uint64) se trompe sur %d des 2000 messages\n", pub.N, erreurs_phase1);
    VERIF(erreurs_phase1 > 0);
}

static void test_little_endian(void) {
    /* Avec E = 1, le chiffre vaut le clair : on voit directement l'ordre des octets. */
    rsaKey_t id = { 1, 4294967311ULL };
    const uint8_t b[BLOCK_SIZE] = { 0x78, 0x56, 0x34, 0x12 };
    uint64_t c = 0;
    VERIF(chiffrer_bloc_gmp(b, &id, &c) == RSA_SUCCES);
    VERIF(c == 0x12345678ULL);                          /* octet 0 = poids faible */
    VERIF(convert_4byte2int(b) == 0x12345678u);
    uint8_t out[BLOCK_SIZE];
    convertInt2uchar(0x12345678u, out);
    VERIF(memcmp(out, b, BLOCK_SIZE) == 0);
    /* signature imposee par le sujet */
    mpz_t r;
    mpz_init(r);
    puissance_mod_n_gmp(r, 2, 10, 1000);
    VERIF(mpz_cmp_ui(r, 24) == 0);                      /* 1024 mod 1000 */
    puissance_mod_n_gmp(r, 5, 3, 0);
    VERIF(mpz_cmp_ui(r, 0) == 0);
    mpz_clear(r);
}

static void test_gen_cles(void) {
    rsaKey_t pub, priv;
    VERIF(gen_keys_gmp(NULL, &priv, 20) == RSA_ERREUR_PARAMETRE);
    VERIF(gen_keys_gmp(&pub, NULL, 20) == RSA_ERREUR_PARAMETRE);
    VERIF(gen_keys_gmp(&pub, &pub, 20) == RSA_ERREUR_PARAMETRE);
    VERIF(gen_keys_gmp(&pub, &priv, 16) == RSA_ERREUR_PARAMETRE);
    VERIF(gen_keys_gmp(&pub, &priv, 32) == RSA_ERREUR_PARAMETRE);
    for (unsigned bits = RSA_BITS_PREMIER_MIN; bits <= RSA_BITS_PREMIER_MAX; bits++) {
        for (int k = 0; k < 40; k++) {
            VERIF(gen_keys_gmp(&pub, &priv, bits) == RSA_SUCCES);
            VERIF(pub.N == priv.N);
            VERIF(pub.N > UINT32_MAX);
            VERIF(cle_bloc_valide(&pub) && cle_bloc_valide(&priv));
        }
    }
}

static void test_blocs(void) {
    static const uint8_t cas[][BLOCK_SIZE] = {
        {0, 0, 0, 0}, {0xFF, 0xFF, 0xFF, 0xFF}, {0, 0, 0, 1}, {1, 0, 0, 0},
        {'T', 'e', 's', 't'}, {0xC3, 0xA9, 0xC3, 0xA8}   /* "éè" en UTF-8 */
    };
    for (unsigned bits = RSA_BITS_PREMIER_MIN; bits <= RSA_BITS_PREMIER_MAX; bits += 2) {
        for (int k = 0; k < 100; k++) {
            rsaKey_t pub, priv;
            VERIF(gen_keys_gmp(&pub, &priv, bits) == RSA_SUCCES);
            for (size_t i = 0; i < sizeof cas / sizeof cas[0]; i++) {
                uint64_t c; uint8_t clair[BLOCK_SIZE];
                VERIF(chiffrer_bloc_gmp(cas[i], &pub, &c) == RSA_SUCCES);
                VERIF(c < pub.N);
                VERIF(dechiffrer_bloc_gmp(c, &priv, clair) == RSA_SUCCES);
                VERIF(memcmp(clair, cas[i], BLOCK_SIZE) == 0);
            }
            for (int r = 0; r < 20; r++) {   /* blocs aleatoires */
                uint8_t b[BLOCK_SIZE], clair[BLOCK_SIZE]; uint64_t c;
                for (int j = 0; j < BLOCK_SIZE; j++) b[j] = (uint8_t)random_uint(0, 255);
                VERIF(chiffrer_bloc_gmp(b, &pub, &c) == RSA_SUCCES);
                VERIF(dechiffrer_bloc_gmp(c, &priv, clair) == RSA_SUCCES);
                VERIF(memcmp(clair, b, BLOCK_SIZE) == 0);
            }
        }
    }
}

static void test_erreurs(void) {
    rsaKey_t pub, priv, petite = { 3, 1000003ULL }, nulle = { 0, 4294967311ULL };
    uint8_t b[BLOCK_SIZE] = {1, 2, 3, 4}, sortie[BLOCK_SIZE] = {9, 9, 9, 9};
    uint64_t c;
    VERIF(gen_keys_gmp(&pub, &priv, 25) == RSA_SUCCES);
    VERIF(chiffrer_bloc_gmp(NULL, &pub, &c) == RSA_ERREUR_PARAMETRE);
    VERIF(chiffrer_bloc_gmp(b, NULL, &c) == RSA_ERREUR_PARAMETRE);
    VERIF(chiffrer_bloc_gmp(b, &pub, NULL) == RSA_ERREUR_PARAMETRE);
    VERIF(chiffrer_bloc_gmp(b, &petite, &c) == RSA_ERREUR_CLE);   /* N <= 2^32 : un bloc ne tient pas */
    VERIF(chiffrer_bloc_gmp(b, &nulle, &c) == RSA_ERREUR_CLE);    /* E = 0 */
    VERIF(dechiffrer_bloc_gmp(pub.N, &priv, sortie) == RSA_ERREUR_CHIFFRE);   /* chiffre >= N */
    VERIF(dechiffrer_bloc_gmp(UINT64_MAX, &priv, sortie) == RSA_ERREUR_CHIFFRE);
    VERIF(memcmp(sortie, (uint8_t[]){9, 9, 9, 9}, BLOCK_SIZE) == 0);   /* sortie intacte en cas d'erreur */

    /* mauvaise clef : on ne doit jamais rendre le clair d'origine */
    rsaKey_t pub2, priv2;
    VERIF(gen_keys_gmp(&pub2, &priv2, 25) == RSA_SUCCES);
    VERIF(chiffrer_bloc_gmp(b, &pub, &c) == RSA_SUCCES);
    int r = dechiffrer_bloc_gmp(c, &priv2, sortie);
    VERIF(r != RSA_SUCCES || memcmp(sortie, b, BLOCK_SIZE) != 0);
}

int main(void) {
    init_log();
    test_puissance();
    test_phase1_deborde();
    test_little_endian();
    test_gen_cles();
    test_blocs();
    test_erreurs();
    close_log();
    if (echecs == 0) printf("Phase 2 (Nada) : tous les tests sont passes.\n");
    else printf("Phase 2 (Nada) : %d echec(s).\n", echecs);
    return echecs == 0 ? 0 : 1;
}
