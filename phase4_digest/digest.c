/// \file digest.c
/// \author Nada ARABA
#include <stdio.h>
#include <string.h>
#include "digest.h"
#include "../phase1_adam/chiffrement.h"   /* codes RSA_* */

void digest_octets(const uint8_t *donnees, size_t taille, uint8_t digest[DIGEST_TAILLE]) {
    SHA256_CTX ctx;
    sha256_init(&ctx);
    if (taille > 0) sha256_update(&ctx, donnees, taille);
    sha256_final(&ctx, digest);
}

int digest_fichier(const char *chemin, uint8_t digest[DIGEST_TAILLE]) {
    uint8_t tampon[4096], resultat[DIGEST_TAILLE];
    size_t lus;
    SHA256_CTX ctx;
    if (chemin == NULL || chemin[0] == '\0' || digest == NULL) return RSA_ERREUR_PARAMETRE;
    FILE *f = fopen(chemin, "rb");   /* binaire : le digest porte sur les octets exacts */
    if (f == NULL) return RSA_ERREUR_FICHIER;
    sha256_init(&ctx);
    while ((lus = fread(tampon, 1, sizeof tampon, f)) > 0) sha256_update(&ctx, tampon, lus);
    int erreur = ferror(f);
    if (fclose(f) != 0 || erreur) return RSA_ERREUR_FICHIER;
    sha256_final(&ctx, resultat);
    memcpy(digest, resultat, DIGEST_TAILLE);
    return RSA_SUCCES;
}

void digest_vers_hex(const uint8_t digest[DIGEST_TAILLE], char hex[DIGEST_HEX_TAILLE]) {
    static const char chiffres[] = "0123456789abcdef";
    for (size_t i = 0; i < DIGEST_TAILLE; i++) {
        hex[2 * i] = chiffres[digest[i] >> 4];
        hex[2 * i + 1] = chiffres[digest[i] & 0x0f];
    }
    hex[2 * DIGEST_TAILLE] = '\0';
}

int digest_fichier_hex(const char *chemin, char hex[DIGEST_HEX_TAILLE]) {
    uint8_t digest[DIGEST_TAILLE];
    if (hex == NULL) return RSA_ERREUR_PARAMETRE;
    int res = digest_fichier(chemin, digest);
    if (res == RSA_SUCCES) digest_vers_hex(digest, hex);
    return res;
}
