#include "chiffrement.h"

#ifdef _WIN32
#include <windows.h>
#define TAILLE_NOM_TEMPORAIRE MAX_PATH
#else
#define TAILLE_NOM_TEMPORAIRE 1
#endif

/* Fonction fournie dans rsa_tools.c. */
uint64_t puissance_mod_n(uint64_t a, uint64_t e, uint64_t n);

#define TAILLE_LECTURE 4096u

static int verifier_cle(const rsaKey_t *cle)
{
    if (cle == NULL) {
        return RSA_ERREUR_PARAMETRE;
    }
    /* Avec cette borne, (N - 1)^2 tient dans un uint64_t. */
    if (cle->E == 0 || cle->E >= cle->N || cle->N <= UINT8_MAX ||
        cle->N > RSA_PHASE1_MODULE_MAX) {
        return RSA_ERREUR_CLE;
    }
    return RSA_SUCCES;
}

static void ecrire_uint64(uint64_t valeur, uint8_t *octets)
{
    size_t i;

    for (i = 0; i < RSA_OCTET_CHIFFRE_TAILLE; ++i) {
        octets[i] = (uint8_t)(valeur >> (8u * i));
    }
}

static uint64_t lire_uint64(const uint8_t *octets)
{
    uint64_t valeur = 0;
    size_t i;

    for (i = 0; i < RSA_OCTET_CHIFFRE_TAILLE; ++i) {
        valeur |= (uint64_t)octets[i] << (8u * i);
    }
    return valeur;
}

int chiffrer_octet(uint8_t clair, const rsaKey_t *cle, uint64_t *chiffre)
{
    int resultat;

    if (chiffre == NULL) {
        return RSA_ERREUR_PARAMETRE;
    }
    resultat = verifier_cle(cle);
    if (resultat != RSA_SUCCES) {
        return resultat;
    }
    *chiffre = puissance_mod_n(clair, cle->E, cle->N);
    return RSA_SUCCES;
}

int dechiffrer_octet(uint64_t chiffre, const rsaKey_t *cle, uint8_t *clair)
{
    uint64_t valeur;
    int resultat;

    if (clair == NULL) {
        return RSA_ERREUR_PARAMETRE;
    }
    resultat = verifier_cle(cle);
    if (resultat != RSA_SUCCES) {
        return resultat;
    }
    if (chiffre >= cle->N) {
        return RSA_ERREUR_CHIFFRE;
    }
    valeur = puissance_mod_n(chiffre, cle->E, cle->N);
    if (valeur > UINT8_MAX) {
        return RSA_ERREUR_CHIFFRE;
    }
    *clair = (uint8_t)valeur;
    return RSA_SUCCES;
}

int chiffrer_tableau(const uint8_t *entree, size_t taille,
                    const rsaKey_t *cle, uint8_t *sortie,
                    size_t capacite, size_t *taille_sortie)
{
    size_t i;
    size_t taille_requise;
    int resultat;

    if (taille_sortie == NULL) {
        return RSA_ERREUR_PARAMETRE;
    }
    *taille_sortie = 0;
    resultat = verifier_cle(cle);
    if (resultat != RSA_SUCCES) {
        return resultat;
    }
    if (taille > SIZE_MAX / RSA_OCTET_CHIFFRE_TAILLE) {
        return RSA_ERREUR_TAILLE;
    }
    taille_requise = taille * RSA_OCTET_CHIFFRE_TAILLE;
    if (capacite < taille_requise) {
        return RSA_ERREUR_TAILLE;
    }
    if (taille != 0 && (entree == NULL || sortie == NULL || entree == sortie)) {
        return RSA_ERREUR_PARAMETRE;
    }
    for (i = 0; i < taille; ++i) {
        uint64_t chiffre = puissance_mod_n(entree[i], cle->E, cle->N);
        ecrire_uint64(chiffre, sortie + i * RSA_OCTET_CHIFFRE_TAILLE);
    }
    *taille_sortie = taille_requise;
    return RSA_SUCCES;
}

int dechiffrer_tableau(const uint8_t *entree, size_t taille,
                      const rsaKey_t *cle, uint8_t *sortie,
                      size_t capacite, size_t *taille_sortie)
{
    size_t i;
    size_t nombre_octets;
    int resultat;

    if (taille_sortie == NULL) {
        return RSA_ERREUR_PARAMETRE;
    }
    *taille_sortie = 0;
    resultat = verifier_cle(cle);
    if (resultat != RSA_SUCCES) {
        return resultat;
    }
    if (taille % RSA_OCTET_CHIFFRE_TAILLE != 0) {
        return RSA_ERREUR_CHIFFRE;
    }
    nombre_octets = taille / RSA_OCTET_CHIFFRE_TAILLE;
    if (capacite < nombre_octets) {
        return RSA_ERREUR_TAILLE;
    }
    if (taille != 0 && (entree == NULL || sortie == NULL || entree == sortie)) {
        return RSA_ERREUR_PARAMETRE;
    }
    /* Valider tout le tableau avant d'ecrire le moindre octet de sortie. */
    for (i = 0; i < nombre_octets; ++i) {
        uint8_t clair;
        uint64_t chiffre = lire_uint64(entree + i * RSA_OCTET_CHIFFRE_TAILLE);
        resultat = dechiffrer_octet(chiffre, cle, &clair);
        if (resultat != RSA_SUCCES) {
            return resultat;
        }
    }
    for (i = 0; i < nombre_octets; ++i) {
        uint64_t chiffre = lire_uint64(entree + i * RSA_OCTET_CHIFFRE_TAILLE);
        sortie[i] = (uint8_t)puissance_mod_n(chiffre, cle->E, cle->N);
    }
    *taille_sortie = nombre_octets;
    return RSA_SUCCES;
}

static int chiffrer_flux(FILE *entree, FILE *sortie, const rsaKey_t *cle)
{
    uint8_t clair[TAILLE_LECTURE];
    uint8_t chiffre[TAILLE_LECTURE * RSA_OCTET_CHIFFRE_TAILLE];
    size_t taille;

    while ((taille = fread(clair, 1, sizeof clair, entree)) != 0) {
        size_t taille_chiffre;
        int resultat = chiffrer_tableau(clair, taille, cle, chiffre,
                                       sizeof chiffre, &taille_chiffre);
        if (resultat != RSA_SUCCES) {
            return resultat;
        }
        if (fwrite(chiffre, 1, taille_chiffre, sortie) != taille_chiffre) {
            return RSA_ERREUR_FICHIER;
        }
    }
    return ferror(entree) ? RSA_ERREUR_FICHIER : RSA_SUCCES;
}

static int dechiffrer_flux(FILE *entree, FILE *sortie, const rsaKey_t *cle)
{
    uint8_t chiffre[RSA_OCTET_CHIFFRE_TAILLE];
    size_t taille;

    while ((taille = fread(chiffre, 1, sizeof chiffre, entree)) != 0) {
        uint8_t clair;
        int resultat;

        if (taille != sizeof chiffre) {
            return ferror(entree) ? RSA_ERREUR_FICHIER : RSA_ERREUR_CHIFFRE;
        }
        resultat = dechiffrer_octet(lire_uint64(chiffre), cle, &clair);
        if (resultat != RSA_SUCCES) {
            return resultat;
        }
        if (fwrite(&clair, 1, 1, sortie) != 1) {
            return RSA_ERREUR_FICHIER;
        }
    }
    return ferror(entree) ? RSA_ERREUR_FICHIER : RSA_SUCCES;
}

static int copier_flux(FILE *entree, FILE *sortie)
{
    uint8_t tampon[TAILLE_LECTURE];
    size_t taille;

    while ((taille = fread(tampon, 1, sizeof tampon, entree)) != 0) {
        if (fwrite(tampon, 1, taille, sortie) != taille) {
            return RSA_ERREUR_FICHIER;
        }
    }
    return ferror(entree) ? RSA_ERREUR_FICHIER : RSA_SUCCES;
}

typedef int (*transformation_t)(FILE *, FILE *, const rsaKey_t *);

static FILE *creer_temporaire(char *nom)
{
#ifdef _WIN32
    char dossier[MAX_PATH];
    DWORD taille = GetTempPathA(MAX_PATH, dossier);
    FILE *fichier;

    /* tmpfile() du CRT Windows peut tenter d'ecrire a la racine du disque. */
    if (taille == 0 || taille >= MAX_PATH ||
        GetTempFileNameA(dossier, "rsa", 0, nom) == 0) {
        return NULL;
    }
    fichier = fopen(nom, "wb+");
    if (fichier == NULL) {
        remove(nom);
        nom[0] = '\0';
    }
    return fichier;
#else
    nom[0] = '\0';
    return tmpfile();
#endif
}

static int transformer_fichier(const char *chemin_entree, const char *chemin_sortie,
                               const rsaKey_t *cle, transformation_t transformer)
{
    FILE *entree;
    FILE *temporaire;
    FILE *sortie;
    char nom_temporaire[TAILLE_NOM_TEMPORAIRE] = {0};
    int resultat;

    if (chemin_entree == NULL || chemin_sortie == NULL ||
        chemin_entree[0] == '\0' || chemin_sortie[0] == '\0') {
        return RSA_ERREUR_PARAMETRE;
    }
    resultat = verifier_cle(cle);
    if (resultat != RSA_SUCCES) {
        return resultat;
    }
    entree = fopen(chemin_entree, "rb");
    if (entree == NULL) {
        return RSA_ERREUR_FICHIER;
    }
    temporaire = creer_temporaire(nom_temporaire);
    if (temporaire == NULL) {
        fclose(entree);
        return RSA_ERREUR_FICHIER;
    }
    resultat = transformer(entree, temporaire, cle);
    if (fclose(entree) != 0 && resultat == RSA_SUCCES) {
        resultat = RSA_ERREUR_FICHIER;
    }
    if (resultat == RSA_SUCCES &&
        (fflush(temporaire) != 0 || fseek(temporaire, 0, SEEK_SET) != 0)) {
        resultat = RSA_ERREUR_FICHIER;
    }
    if (resultat == RSA_SUCCES) {
        sortie = fopen(chemin_sortie, "wb");
        if (sortie == NULL) {
            resultat = RSA_ERREUR_FICHIER;
        } else {
            resultat = copier_flux(temporaire, sortie);
            if (fclose(sortie) != 0 && resultat == RSA_SUCCES) {
                resultat = RSA_ERREUR_FICHIER;
            }
        }
    }
    if (fclose(temporaire) != 0 && resultat == RSA_SUCCES) {
        resultat = RSA_ERREUR_FICHIER;
    }
    if (nom_temporaire[0] != '\0' && remove(nom_temporaire) != 0 &&
        resultat == RSA_SUCCES) {
        resultat = RSA_ERREUR_FICHIER;
    }
    return resultat;
}

int chiffrer_fichier(const char *entree, const char *sortie, const rsaKey_t *cle)
{
    return transformer_fichier(entree, sortie, cle, chiffrer_flux);
}

int dechiffrer_fichier(const char *entree, const char *sortie, const rsaKey_t *cle)
{
    return transformer_fichier(entree, sortie, cle, dechiffrer_flux);
}
