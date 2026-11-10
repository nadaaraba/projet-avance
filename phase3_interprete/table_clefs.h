/// \file table_clefs.h
/// \author Nada ARABA
/// \brief Phase 3 : table des clefs de l'utilisateur (table des symboles de l'interprete).
///        Implemente table_chercher de interpreteur.h, plus l'ajout, la suppression et la section
///        binaire utilisee par save / load.
#ifndef TABLE_CLEFS_H
#define TABLE_CLEFS_H

#include <stdio.h>
#include "../phase3_nada/interpreteur.h"

/* Identificateur : 1 a NAME_MAX_SIZE octets, sans espace, caractere de controle ni '/'
 * ('/' separe contact et clef en V2). Memes regles que les contacts d'Adam. */
int table_identifiant_valide(const char *id);

/* Table vide, liee a l'annuaire des contacts (NULL possible). */
void table_initialiser(table_clefs_t *table, const struct annuaire_contacts_s *contacts);
void table_liberer(table_clefs_t *table);

/* Copie l'entree dans la table. CMD_SUCCES, CMD_ERREUR_SYNTAXE (identificateur invalide),
 * CMD_ERREUR_DOUBLON, CMD_ERREUR_MEMOIRE ou RSA_ERREUR_PARAMETRE. */
int table_ajouter(table_clefs_t *table, const entree_clef_t *entree);
/* CMD_SUCCES, CMD_ERREUR_ID_INCONNU ou RSA_ERREUR_PARAMETRE. L'ordre des autres clefs est garde. */
int table_supprimer(table_clefs_t *table, const char *id);

/*
 * Section binaire : "DYKEYS01", nombre de clefs sur 4 octets, puis pour chaque clef :
 * longueur de l'identificateur sur 1 octet et ses octets, type (1 crypt, 2 sign), a_priv (0 ou 1),
 * puis E et N publics et prives sur 8 octets. Entiers en little endian.
 * table_charger ne remplace la table qu'en cas de succes (CMD_ERREUR_FORMAT sinon).
 * Les flux restent ouverts.
 */
int table_sauver(FILE *sortie, const table_clefs_t *table);
int table_charger(FILE *entree, table_clefs_t *table);

#endif
