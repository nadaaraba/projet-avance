/// \file digest.h
/// \author Nada ARABA
/// \brief Phase 4 : digest SHA-256 d'un fichier, en clair ou chiffre (sujet, phase 4, point 2).
///        S'appuie sur sha256.c / sha256_utils.c fournis par le sujet.
#ifndef DIGEST_H
#define DIGEST_H

#include <stdint.h>
#include "sha256.h"

#define DIGEST_TAILLE SHA256_BLOCK_SIZE              /* 32 octets */
#define DIGEST_HEX_TAILLE (2 * SHA256_BLOCK_SIZE + 1) /* 64 caracteres hexadecimaux + '\0' */

/*
 * Digest SHA-256 du contenu exact du fichier (lu en binaire, par morceaux : pas de limite de taille).
 * Retourne RSA_SUCCES, RSA_ERREUR_PARAMETRE ou RSA_ERREUR_FICHIER (absent ou illisible).
 * La sortie n'est modifiee qu'en cas de succes.
 */
int digest_fichier(const char *chemin, uint8_t digest[DIGEST_TAILLE]);

/* Meme chose, en hexadecimal minuscule (format de sha256ofString du sujet). */
int digest_fichier_hex(const char *chemin, char hex[DIGEST_HEX_TAILLE]);

/* Digest d'un tableau d'octets de longueur donnee (les octets nuls sont pris en compte). */
void digest_octets(const uint8_t *donnees, size_t taille, uint8_t digest[DIGEST_TAILLE]);

/* Conversion d'un digest en hexadecimal minuscule, termine par '\0'. */
void digest_vers_hex(const uint8_t digest[DIGEST_TAILLE], char hex[DIGEST_HEX_TAILLE]);

#endif
