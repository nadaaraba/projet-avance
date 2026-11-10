/// \file interprete.h
/// \author Nada ARABA
/// \brief Phase 3 : interprete de commandes de DIYPG (interprete.pdf, V1.1 et V2).
///
/// Commandes : quit, help, listkeys [<keyid>], rmkeys <keyid>, newkeys <keyid> <type>,
/// crypt / uncrypt <fileIn> <fileOut> <keyid>, bin-2b64 / b64-2bin <filein> <fileout>,
/// save [<fileout>], savepub <keyid> <file>, load [<filein>], show <keyid> ["pub"] ["priv"],
/// et les commandes de contacts d'Adam (listcontacts, addcontact, modifycontact, addkeys,
/// rmcontact, rmkeys contact/clef).
#ifndef INTERPRETE_H
#define INTERPRETE_H

#include <stdio.h>
#include "table_clefs.h"
#include "../phase3_adam/contacts.h"

#define INTERPRETE_QUITTER 2                 /* retour de "quit" : fin de la boucle */
#define INTERPRETE_SAUVEGARDE "diypg.sav"    /* fichier par defaut de save / load */
#define INTERPRETE_CLEF_DEFAUT "diypg_defaut.key"
#define INTERPRETE_ARGS_MAX 16

typedef struct {
    table_clefs_t table;               /* table.contacts pointe vers contacts */
    annuaire_contacts_t contacts;
    const char *sauvegarde;            /* fichier par defaut de save / load */
    const char *clef_defaut;           /* clef par defaut qui chiffre la sauvegarde */
} interprete_t;

void interprete_initialiser(interprete_t *it);
void interprete_liberer(interprete_t *it);

/*
 * Execute une commande deja decoupee (argv[0] = nom). entree sert aux menus de saisie des contacts.
 * Retourne 0, INTERPRETE_QUITTER, ou un code negatif (RSA_*, CMD_*, CONTACTS_*).
 */
int interprete_executer(interprete_t *it, int argc, char **argv, FILE *entree, FILE *sortie);

/* Decoupe une ligne (espaces et tabulations ; '#' en debut de ligne = commentaire) puis l'execute.
 * La ligne est modifiee. Une ligne vide ne fait rien. */
int interprete_ligne(interprete_t *it, char *ligne, FILE *entree, FILE *sortie);

/*
 * Boucle de lecture jusqu'a quit ou la fin de l'entree. echo recopie chaque commande (rejeu d'un
 * script), invite affiche "diypg> ". Chaque erreur est affichee sur sortie.
 * Retourne le nombre de commandes en erreur.
 */
int interprete_boucle(interprete_t *it, FILE *entree, FILE *sortie, int echo, int invite);

/* Texte d'un code de retour, quel que soit le module qui l'a produit. */
const char *interprete_message(int code);

#endif
