/// \file gmp_rsa.c
/// \author Nada ARABA
#include <gmp.h>
#include <time.h>
#include "gmp_rsa.h"

/* ---------- conversions uint64_t <-> mpz_t (portables, sans dependre de unsigned long) ---------- */
static void u64_vers_mpz(mpz_t z, uint64_t v) {
    mpz_import(z, 1, 1, sizeof v, 0, 0, &v);   /* v == 0 donne bien 0 */
}

static int mpz_vers_u64(uint64_t *v, const mpz_t z) {
    size_t n = 0;
    if (mpz_sgn(z) < 0 || mpz_sizeinbase(z, 2) > 64) return -1;
    *v = 0;
    mpz_export(v, &n, 1, sizeof *v, 0, 0, z);   /* z == 0 : n == 0 et *v reste 0 */
    return 0;
}

void puissance_mod_n_gmp(mpz_t res, uint64_t a, uint64_t e, uint64_t n) {
    mpz_t za, ze, zn;
    if (n == 0) {
        mpz_set_ui(res, 0);
        return;
    }
    mpz_inits(za, ze, zn, NULL);
    u64_vers_mpz(za, a);
    u64_vers_mpz(ze, e);
    u64_vers_mpz(zn, n);
    mpz_powm(res, za, ze, zn);
    mpz_clears(za, ze, zn, NULL);
}

uint64_t puissance_mod_n_gmp_u64(uint64_t a, uint64_t e, uint64_t n) {
    mpz_t r;
    uint64_t v = 0;
    mpz_init(r);
    puissance_mod_n_gmp(r, a, e, n);
    mpz_vers_u64(&v, r);   /* r < n < 2^64 : ne peut pas echouer */
    mpz_clear(r);
    return v;
}

/* ---------- generation de clefs ---------- */
static gmp_randstate_t etat_alea;
static int etat_initialise = 0;

static void init_alea(void) {
    if (etat_initialise) return;
    gmp_randinit_default(etat_alea);
    /* Graine : heure + horloge CPU + adresse d'une variable (suffisant pour un projet pedagogique). */
    unsigned long graine = (unsigned long)time(NULL) * 1000003UL
                         ^ (unsigned long)clock()
                         ^ (unsigned long)(uintptr_t)&etat_initialise;
    gmp_randseed_ui(etat_alea, graine);
    etat_initialise = 1;
}

/* Tire un premier de exactement `bits` bits (bit de poids fort force a 1). */
static void tire_premier(mpz_t p, unsigned bits) {
    mpz_urandomb(p, etat_alea, bits - 1);
    mpz_setbit(p, bits - 1);
    mpz_nextprime(p, p);   /* peut depasser de peu 2^bits : sans importance, reste < 2^32 pour bits <= 31 */
}

int gen_keys_gmp(rsaKey_t *pub, rsaKey_t *priv, unsigned bits_premier) {
    mpz_t p, q, n, phi, e, d, g;
    int res = RSA_SUCCES;
    uint64_t vn = 0, ve = 0, vd = 0;

    if (pub == NULL || priv == NULL || pub == priv ||
        bits_premier < RSA_BITS_PREMIER_MIN || bits_premier > RSA_BITS_PREMIER_MAX)
        return RSA_ERREUR_PARAMETRE;

    init_alea();
    mpz_inits(p, q, n, phi, e, d, g, NULL);

    tire_premier(p, bits_premier);
    do {
        tire_premier(q, bits_premier);
    } while (mpz_cmp(p, q) == 0);

    mpz_mul(n, p, q);
    mpz_sub_ui(p, p, 1);
    mpz_sub_ui(q, q, 1);
    mpz_mul(phi, p, q);

    /* E = 65537, sinon le premier impair suivant qui est premier avec phi */
    mpz_set_ui(e, 65537);
    for (;;) {
        mpz_gcd(g, e, phi);
        if (mpz_cmp_ui(g, 1) == 0) break;
        mpz_add_ui(e, e, 2);
    }
    if (mpz_invert(d, e, phi) == 0 ||
        mpz_vers_u64(&vn, n) != 0 || mpz_vers_u64(&ve, e) != 0 || mpz_vers_u64(&vd, d) != 0) {
        res = RSA_ERREUR_CLE;   /* ne devrait jamais arriver */
    } else {
        pub->E = ve;  pub->N = vn;
        priv->E = vd; priv->N = vn;
    }
    mpz_clears(p, q, n, phi, e, d, g, NULL);
    return res;
}

/* ---------- blocs de 4 octets ---------- */
int cle_bloc_valide(const rsaKey_t *cle) {
    return cle != NULL && cle->E > 0 && cle->E < cle->N && cle->N > UINT32_MAX;
}

int chiffrer_bloc_gmp(const uint8_t bloc[BLOCK_SIZE], const rsaKey_t *cle, uint64_t *chiffre) {
    if (bloc == NULL || cle == NULL || chiffre == NULL) return RSA_ERREUR_PARAMETRE;
    if (!cle_bloc_valide(cle)) return RSA_ERREUR_CLE;
    uint64_t m = convert_4byte2int(bloc);   /* little endian */
    *chiffre = puissance_mod_n_gmp_u64(m, cle->E, cle->N);
    return RSA_SUCCES;
}

int dechiffrer_bloc_gmp(uint64_t chiffre, const rsaKey_t *cle, uint8_t bloc[BLOCK_SIZE]) {
    if (bloc == NULL || cle == NULL) return RSA_ERREUR_PARAMETRE;
    if (!cle_bloc_valide(cle)) return RSA_ERREUR_CLE;
    if (chiffre >= cle->N) return RSA_ERREUR_CHIFFRE;
    uint64_t m = puissance_mod_n_gmp_u64(chiffre, cle->E, cle->N);
    if (m > UINT32_MAX) return RSA_ERREUR_CHIFFRE;
    convertInt2uchar((uint32_t)m, bloc);
    return RSA_SUCCES;
}
