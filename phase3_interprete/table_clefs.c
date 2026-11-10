/// \file table_clefs.c
/// \author Nada ARABA
#include <stdlib.h>
#include <string.h>
#include "table_clefs.h"
#include "../phase1_adam/chiffrement.h"   /* RSA_ERREUR_PARAMETRE, RSA_ERREUR_FICHIER */

static const uint8_t signature_clefs[8] = {'D', 'Y', 'K', 'E', 'Y', 'S', '0', '1'};

int table_identifiant_valide(const char *id) {
    size_t taille;
    if (id == NULL) return 0;
    taille = strlen(id);
    if (taille == 0 || taille > NAME_MAX_SIZE) return 0;
    for (size_t i = 0; i < taille; i++) {
        unsigned char c = (unsigned char)id[i];
        if (c <= 32 || c == 127 || c == '/') return 0;
    }
    return 1;
}

const entree_clef_t *table_chercher(const table_clefs_t *table, const char *id) {
    if (table == NULL || id == NULL || (table->entrees == NULL && table->nb > 0)) return NULL;
    for (size_t i = 0; i < table->nb; i++) {
        if (strcmp(table->entrees[i].id, id) == 0) return &table->entrees[i];
    }
    return NULL;
}

void table_initialiser(table_clefs_t *table, const struct annuaire_contacts_s *contacts) {
    if (table == NULL) return;
    table->entrees = NULL;
    table->nb = 0;
    table->capacite = 0;
    table->contacts = contacts;
}

void table_liberer(table_clefs_t *table) {
    if (table == NULL) return;
    free(table->entrees);
    table->entrees = NULL;
    table->nb = 0;
    table->capacite = 0;
}

int table_ajouter(table_clefs_t *table, const entree_clef_t *entree) {
    if (table == NULL || entree == NULL) return RSA_ERREUR_PARAMETRE;
    if (!table_identifiant_valide(entree->id) ||
        (entree->type != CLEF_CRYPT && entree->type != CLEF_SIGN)) return CMD_ERREUR_SYNTAXE;
    if (table_chercher(table, entree->id) != NULL) return CMD_ERREUR_DOUBLON;
    if (table->nb == table->capacite) {
        size_t capacite = table->capacite == 0 ? 8 : 2 * table->capacite;
        if (capacite > SIZE_MAX / sizeof *table->entrees) return CMD_ERREUR_MEMOIRE;
        entree_clef_t *entrees = realloc(table->entrees, capacite * sizeof *entrees);
        if (entrees == NULL) return CMD_ERREUR_MEMOIRE;
        table->entrees = entrees;
        table->capacite = capacite;
    }
    table->entrees[table->nb++] = *entree;
    return CMD_SUCCES;
}

int table_supprimer(table_clefs_t *table, const char *id) {
    if (table == NULL || id == NULL) return RSA_ERREUR_PARAMETRE;
    for (size_t i = 0; i < table->nb; i++) {
        if (strcmp(table->entrees[i].id, id) == 0) {
            memmove(&table->entrees[i], &table->entrees[i + 1],
                    (table->nb - i - 1) * sizeof *table->entrees);
            table->nb--;
            return CMD_SUCCES;
        }
    }
    return CMD_ERREUR_ID_INCONNU;
}

static int ecrire_nombre(FILE *f, uint64_t valeur, size_t taille) {
    uint8_t octets[8];
    for (size_t i = 0; i < taille; i++) octets[i] = (uint8_t)(valeur >> (8u * i));
    return fwrite(octets, 1, taille, f) == taille;
}

static int lire_nombre(FILE *f, uint64_t *valeur, size_t taille) {
    uint8_t octets[8];
    if (fread(octets, 1, taille, f) != taille) return 0;
    *valeur = 0;
    for (size_t i = 0; i < taille; i++) *valeur |= (uint64_t)octets[i] << (8u * i);
    return 1;
}

int table_sauver(FILE *sortie, const table_clefs_t *table) {
    if (sortie == NULL || table == NULL || (table->entrees == NULL && table->nb > 0)) return RSA_ERREUR_PARAMETRE;
    if (table->nb > UINT32_MAX) return CMD_ERREUR_FORMAT;
    int ok = fwrite(signature_clefs, 1, sizeof signature_clefs, sortie) == sizeof signature_clefs &&
             ecrire_nombre(sortie, table->nb, 4);
    for (size_t i = 0; ok && i < table->nb; i++) {
        const entree_clef_t *e = &table->entrees[i];
        size_t taille = strlen(e->id);
        ok = ecrire_nombre(sortie, taille, 1) && fwrite(e->id, 1, taille, sortie) == taille &&
             ecrire_nombre(sortie, e->type == CLEF_SIGN ? 2 : 1, 1) && ecrire_nombre(sortie, e->a_priv ? 1 : 0, 1) &&
             ecrire_nombre(sortie, e->pub.E, 8) && ecrire_nombre(sortie, e->pub.N, 8) &&
             ecrire_nombre(sortie, e->a_priv ? e->priv.E : 0, 8) && ecrire_nombre(sortie, e->a_priv ? e->priv.N : 0, 8);
    }
    return ok && !ferror(sortie) ? CMD_SUCCES : RSA_ERREUR_FICHIER;
}

/* Lit une entree ; 0 si la section est tronquee ou invalide. */
static int lire_entree(FILE *f, entree_clef_t *e) {
    uint64_t taille, type, a_priv;
    memset(e, 0, sizeof *e);
    if (!lire_nombre(f, &taille, 1) || taille == 0 || taille > NAME_MAX_SIZE ||
        fread(e->id, 1, (size_t)taille, f) != taille) return 0;
    e->id[taille] = '\0';
    if (!lire_nombre(f, &type, 1) || !lire_nombre(f, &a_priv, 1) ||
        !lire_nombre(f, &e->pub.E, 8) || !lire_nombre(f, &e->pub.N, 8) ||
        !lire_nombre(f, &e->priv.E, 8) || !lire_nombre(f, &e->priv.N, 8)) return 0;
    if ((type != 1 && type != 2) || a_priv > 1) return 0;
    if (e->pub.N <= 1 || e->pub.E == 0 || e->pub.E >= e->pub.N) return 0;
    if (a_priv && (e->priv.N != e->pub.N || e->priv.E == 0 || e->priv.E >= e->priv.N)) return 0;
    e->type = type == 2 ? CLEF_SIGN : CLEF_CRYPT;
    e->a_priv = a_priv == 1;
    return 1;
}

int table_charger(FILE *entree, table_clefs_t *table) {
    uint8_t signature[8];
    uint64_t nombre;
    table_clefs_t temporaire;
    int res = CMD_SUCCES;
    if (entree == NULL || table == NULL) return RSA_ERREUR_PARAMETRE;
    if (fread(signature, 1, sizeof signature, entree) != sizeof signature ||
        memcmp(signature, signature_clefs, sizeof signature) != 0 || !lire_nombre(entree, &nombre, 4))
        return ferror(entree) ? RSA_ERREUR_FICHIER : CMD_ERREUR_FORMAT;
    table_initialiser(&temporaire, table->contacts);
    for (uint64_t i = 0; i < nombre && res == CMD_SUCCES; i++) {
        entree_clef_t e;
        if (!lire_entree(entree, &e)) res = CMD_ERREUR_FORMAT;
        else {
            res = table_ajouter(&temporaire, &e);
            if (res != CMD_SUCCES && res != CMD_ERREUR_MEMOIRE) res = CMD_ERREUR_FORMAT;   /* doublon, id invalide */
        }
    }
    if (ferror(entree)) res = RSA_ERREUR_FICHIER;
    if (res != CMD_SUCCES) {
        table_liberer(&temporaire);
        return res;
    }
    table_liberer(table);
    *table = temporaire;
    return CMD_SUCCES;
}
