#ifndef CHIFFREMENT_PHASE1_H
#define CHIFFREMENT_PHASE1_H

#include <stddef.h>
#include "../rsa_common_header.h"

/* Un octet clair donne un entier RSA encode sur 8 octets little endian. */
#define RSA_OCTET_CHIFFRE_TAILLE 8u
#define RSA_PHASE1_MODULE_MAX UINT64_C(4294967296)

typedef enum {
    RSA_SUCCES = 0,
    RSA_ERREUR_PARAMETRE = -1,
    RSA_ERREUR_CLE = -2,
    RSA_ERREUR_TAILLE = -3,
    RSA_ERREUR_CHIFFRE = -4,
    RSA_ERREUR_FICHIER = -5
} rsa_resultat_t;

/* La cle doit verifier 0 < E < N et 255 < N <= 2^32. */
int chiffrer_octet(uint8_t clair, const rsaKey_t *cle, uint64_t *chiffre);
int dechiffrer_octet(uint64_t chiffre, const rsaKey_t *cle, uint8_t *clair);

/*
 * Les tableaux d'entree et de sortie ne doivent pas se recouvrir.
 * Le demandeur alloue la sortie : 8 * taille octets pour chiffrer,
 * taille / 8 octets pour dechiffrer. Aucun terminateur '\0' n'est ajoute.
 * Une entree vide accepte des tableaux NULL. taille_sortie reste obligatoire.
 * En cas d'erreur, taille_sortie vaut 0 et la sortie reste inchangee.
 */
int chiffrer_tableau(const uint8_t *entree, size_t taille,
                    const rsaKey_t *cle, uint8_t *sortie,
                    size_t capacite, size_t *taille_sortie);
int dechiffrer_tableau(const uint8_t *entree, size_t taille,
                      const rsaKey_t *cle, uint8_t *sortie,
                      size_t capacite, size_t *taille_sortie);

/*
 * Fichiers binaires, meme format que les tableaux.
 * L'entree est traitee dans un fichier temporaire avant d'ouvrir la sortie.
 * Un chiffre invalide ne remplace donc pas un fichier de sortie existant.
 * Les chemins d'entree et de sortie peuvent designer le meme fichier.
 */
int chiffrer_fichier(const char *entree, const char *sortie, const rsaKey_t *cle);
int dechiffrer_fichier(const char *entree, const char *sortie, const rsaKey_t *cle);

#endif
