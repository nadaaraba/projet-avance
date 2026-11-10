#include "chiffrement_blocs.h"
#include "../phase2_nada/gmp_rsa.h"

static int chiffrer_entier(uint64_t clair, const rsaKey_t *cle, uint64_t *chiffre)
{
    uint8_t bloc[BLOCK_SIZE];
    convertInt2uchar((uint32_t)clair, bloc);
    return chiffrer_bloc_gmp(bloc, cle, chiffre);
}

static int dechiffrer_entier(uint64_t chiffre, const rsaKey_t *cle, uint64_t *clair)
{
    uint8_t bloc[BLOCK_SIZE];
    int resultat = dechiffrer_bloc_gmp(chiffre, cle, bloc);
    if (resultat == RSA_SUCCES) {
        *clair = convert_4byte2int(bloc);
    }
    return resultat;
}

int chiffrer_fichier_blocs_gmp(const char *entree, const char *sortie,
                              const rsaKey_t *cle)
{
    return chiffrer_fichier_blocs(entree, sortie, cle, chiffrer_entier);
}

int dechiffrer_fichier_blocs_gmp(const char *entree, const char *sortie,
                                const rsaKey_t *cle)
{
    return dechiffrer_fichier_blocs(entree, sortie, cle, dechiffrer_entier);
}
