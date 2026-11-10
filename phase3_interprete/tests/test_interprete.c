/// \file test_interprete.c
/// \author Nada ARABA
/// \brief Tests de la table des clefs et de l'interprete : commandes V1, erreurs (identificateur
///        inconnu, mauvais type, fichier absent), save / load chiffres et atomiques.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../interprete.h"
#include "../../phase1_nada/phase1/phase1.h"
#include "../../phase1_adam/chiffrement.h"
#include "../../phase1_loann/base64.h"
#include "../../console_utf8.h"

static unsigned controles = 0, echecs = 0;
#define VERIF(cond) do { ++controles; \
    if (!(cond)) { fprintf(stderr, "ECHEC interprete ligne %d : %s\n", __LINE__, #cond); ++echecs; } } while (0)

/* Execute une ligne de commande ; la sortie va dans sortie.txt, relue dans texte. */
static char texte[4096];
static int commande(interprete_t *it, const char *ligne) {
    char copie[512];
    FILE *entree = fopen("formulaire.txt", "w+"), *sortie = fopen("sortie.txt", "w+");
    if (entree == NULL || sortie == NULL) return -1000;
    snprintf(copie, sizeof copie, "%s", ligne);
    int res = interprete_ligne(it, copie, entree, sortie);
    rewind(sortie);
    size_t n = fread(texte, 1, sizeof texte - 1, sortie);
    texte[n] = '\0';
    fclose(entree);
    fclose(sortie);
    return res;
}

static entree_clef_t entree(const char *id, type_clef_t type, uint64_t n) {
    entree_clef_t e;
    memset(&e, 0, sizeof e);
    snprintf(e.id, sizeof e.id, "%s", id);
    e.type = type;
    e.pub = (rsaKey_t){ 3, n };
    e.priv = (rsaKey_t){ 7, n };
    e.a_priv = true;
    return e;
}

static size_t lire(const char *chemin, unsigned char *octets, size_t capacite) {
    FILE *f = fopen(chemin, "rb");
    if (f == NULL) return 0;
    size_t n = fread(octets, 1, capacite, f);
    fclose(f);
    return n;
}

static int ecrire(const char *chemin, const unsigned char *octets, size_t taille) {
    FILE *f = fopen(chemin, "wb");
    if (f == NULL) return 0;
    int ok = fwrite(octets, 1, taille, f) == taille;
    return fclose(f) == 0 && ok;
}

static void test_table(void) {
    table_clefs_t t;
    entree_clef_t e = entree("perso", CLEF_CRYPT, 4295229443ULL);
    char long_id[NAME_MAX_SIZE + 2];
    memset(long_id, 'a', sizeof long_id - 1);
    long_id[sizeof long_id - 1] = '\0';
    table_initialiser(&t, NULL);
    VERIF(table_identifiant_valide("perso") && table_identifiant_valide("clé_é"));   /* UTF-8 accepte */
    VERIF(!table_identifiant_valide("") && !table_identifiant_valide("a b") && !table_identifiant_valide("a/b"));
    VERIF(!table_identifiant_valide(long_id) && !table_identifiant_valide(NULL));
    long_id[NAME_MAX_SIZE] = '\0';
    VERIF(table_identifiant_valide(long_id));
    VERIF(table_ajouter(&t, &e) == CMD_SUCCES);
    VERIF(table_ajouter(&t, &e) == CMD_ERREUR_DOUBLON);
    VERIF(table_ajouter(NULL, &e) == RSA_ERREUR_PARAMETRE && table_ajouter(&t, NULL) == RSA_ERREUR_PARAMETRE);
    entree_clef_t mauvais = entree("a b", CLEF_CRYPT, 4295229443ULL);
    VERIF(table_ajouter(&t, &mauvais) == CMD_ERREUR_SYNTAXE);
    for (int i = 0; i < 20; i++) {   /* plusieurs agrandissements du tableau */
        char id[16];
        snprintf(id, sizeof id, "k%d", i);
        entree_clef_t k = entree(id, i % 2 ? CLEF_SIGN : CLEF_CRYPT, 4295229443ULL + (uint64_t)i);
        VERIF(table_ajouter(&t, &k) == CMD_SUCCES);
    }
    VERIF(t.nb == 21 && t.capacite >= 21);
    VERIF(table_chercher(&t, "k13") != NULL && table_chercher(&t, "k13")->type == CLEF_SIGN);
    VERIF(table_supprimer(&t, "k5") == CMD_SUCCES && table_chercher(&t, "k5") == NULL && t.nb == 20);
    VERIF(strcmp(t.entrees[5].id, "k4") == 0 && strcmp(t.entrees[6].id, "k6") == 0);   /* ordre garde */
    VERIF(table_supprimer(&t, "k5") == CMD_ERREUR_ID_INCONNU);

    /* section binaire : aller-retour, puis chaque coupure refusee sans toucher a la table */
    FILE *f = fopen("table.bin", "wb");
    VERIF(f != NULL && table_sauver(f, &t) == CMD_SUCCES);
    if (f != NULL) fclose(f);
    table_clefs_t relue, garde;
    table_initialiser(&relue, NULL);
    table_initialiser(&garde, NULL);
    VERIF(table_ajouter(&garde, &e) == CMD_SUCCES);
    f = fopen("table.bin", "rb");
    VERIF(f != NULL && table_charger(f, &relue) == CMD_SUCCES);
    if (f != NULL) fclose(f);
    VERIF(relue.nb == t.nb && memcmp(relue.entrees, t.entrees, t.nb * sizeof *t.entrees) == 0);
    static unsigned char octets[4096];
    size_t taille = lire("table.bin", octets, sizeof octets);
    VERIF(taille > 12);
    for (size_t i = 0; i < taille; i += 7) {
        VERIF(ecrire("coupe.bin", octets, i));
        f = fopen("coupe.bin", "rb");
        VERIF(f != NULL && table_charger(f, &garde) == CMD_ERREUR_FORMAT);
        if (f != NULL) fclose(f);
        VERIF(garde.nb == 1 && table_chercher(&garde, "perso") != NULL);
    }
    octets[12 + 1 + 5] = 9;   /* type de la premiere clef ("perso" : 5 octets) */
    VERIF(ecrire("coupe.bin", octets, taille));
    f = fopen("coupe.bin", "rb");
    VERIF(f != NULL && table_charger(f, &garde) == CMD_ERREUR_FORMAT);
    if (f != NULL) fclose(f);
    table_liberer(&t);
    table_liberer(&relue);
    table_liberer(&garde);
}

static void test_commandes(void) {
    interprete_t it;
    interprete_initialiser(&it);
    VERIF(commande(&it, "listkeys") == CMD_SUCCES && strcmp(texte, "Aucune clef.\n") == 0);
    VERIF(commande(&it, "newkeys perso crypt") == CMD_SUCCES);
    VERIF(commande(&it, "newkeys signature sign") == CMD_SUCCES);
    VERIF(commande(&it, "listkeys") == CMD_SUCCES && strcmp(texte, "perso (crypt)\nsignature (sign)\n") == 0);
    VERIF(commande(&it, "listkeys signature") == CMD_SUCCES && strcmp(texte, "signature (sign)\n") == 0);
    VERIF(commande(&it, "listkeys absente") == CMD_ERREUR_ID_INCONNU);
    VERIF(commande(&it, "listkeys a b") == CMD_ERREUR_SYNTAXE);
    const entree_clef_t *e = table_chercher(&it.table, "perso");
    VERIF(e != NULL && e->a_priv && e->pub.N > UINT32_MAX && e->pub.N == e->priv.N);   /* clefs GMP */
    VERIF(commande(&it, "newkeys perso crypt") == CMD_ERREUR_DOUBLON);
    VERIF(commande(&it, "newkeys autre rsa") == CMD_ERREUR_TYPE);
    VERIF(commande(&it, "newkeys a/b crypt") == CMD_ERREUR_SYNTAXE);
    VERIF(commande(&it, "newkeys seul") == CMD_ERREUR_SYNTAXE);
    VERIF(commande(&it, "rmkeys absente") == CMD_ERREUR_ID_INCONNU);
    VERIF(commande(&it, "rmkeys") == CMD_ERREUR_SYNTAXE);
    VERIF(commande(&it, "bin-2b64 fichier_absent.bin x.b64") == RSA_ERREUR_FICHIER);
    VERIF(commande(&it, "b64-2bin fichier_absent.b64 x.bin") == RSA_ERREUR_FICHIER);
    VERIF(commande(&it, "bin-2b64 un") == CMD_ERREUR_SYNTAXE);
    VERIF(commande(&it, "quit") == INTERPRETE_QUITTER && commande(&it, "quit maintenant") == CMD_ERREUR_SYNTAXE);
    VERIF(commande(&it, "inconnue") == CMD_ERREUR_INCONNUE);
    VERIF(commande(&it, "") == CMD_SUCCES && commande(&it, "   # commentaire") == CMD_SUCCES);
    VERIF(commande(&it, "help") == CMD_SUCCES && strstr(texte, "newkeys <keyid>") != NULL);
    VERIF(commande(&it, "a a a a a a a a a a a a a a a a a") == CMD_ERREUR_SYNTAXE);   /* 17 mots */
    VERIF(interprete_executer(NULL, 1, (char *[]){ "help" }, stdin, stdout) == RSA_ERREUR_PARAMETRE);
    VERIF(interprete_ligne(&it, NULL, stdin, stdout) == RSA_ERREUR_PARAMETRE);

    /* Base64 aller-retour et chiffrement par l'interprete */
    VERIF(ecrire("clair.bin", (const unsigned char *)"\x00\x01\xff texte", 9));
    VERIF(commande(&it, "crypt clair.bin chiffre.bin perso") == RSA_SUCCES);
    VERIF(commande(&it, "bin-2b64 chiffre.bin chiffre.b64") == CMD_SUCCES);
    VERIF(commande(&it, "b64-2bin chiffre.b64 recu.bin") == CMD_SUCCES);
    VERIF(commande(&it, "uncrypt recu.bin recu_clair.bin perso") == RSA_SUCCES);
    unsigned char a[64], b[64];
    VERIF(lire("clair.bin", a, sizeof a) == 9 && lire("recu_clair.bin", b, sizeof b) == 9 && memcmp(a, b, 9) == 0);
    VERIF(commande(&it, "crypt clair.bin x.bin signature") == CMD_ERREUR_TYPE);

    /* V2 : les commandes de contacts passent par l'interprete */
    FILE *formulaire = fopen("menu.txt", "w+");
    VERIF(formulaire != NULL);
    if (formulaire != NULL) {
        fputs("Dupuis\nJean\nvoisin\nboulot\ncrypt\n10001\n100040003\n", formulaire);
        rewind(formulaire);
        char l1[] = "addcontact jean", l2[] = "addkeys jean";
        VERIF(interprete_ligne(&it, l1, formulaire, stdout) == CONTACTS_SUCCES);
        VERIF(interprete_ligne(&it, l2, formulaire, stdout) == CONTACTS_SUCCES);
        fclose(formulaire);
        printf("\n");
    }
    VERIF(commande(&it, "crypt clair.bin pour_jean.bin jean") == RSA_SUCCES);
    VERIF(commande(&it, "uncrypt pour_jean.bin x.bin jean") == CMD_ERREUR_PAS_DE_PRIVEE);
    VERIF(commande(&it, "rmkeys jean/boulot") == CONTACTS_SUCCES);
    VERIF(commande(&it, "crypt clair.bin x.bin jean") == CMD_ERREUR_ID_INCONNU);

    /* messages : chaque code connu a un texte */
    static const int codes[] = { RSA_ERREUR_PARAMETRE, RSA_ERREUR_CLE, RSA_ERREUR_TAILLE, RSA_ERREUR_CHIFFRE,
        RSA_ERREUR_FICHIER, CMD_ERREUR_SYNTAXE, CMD_ERREUR_ID_INCONNU, CMD_ERREUR_TYPE, CMD_ERREUR_PAS_DE_PRIVEE,
        CMD_ERREUR_AMBIGU, CMD_ERREUR_DOUBLON, CMD_ERREUR_MEMOIRE, CMD_ERREUR_FORMAT, CMD_ERREUR_INCONNUE,
        CONTACTS_ERREUR_INTROUVABLE, CONTACTS_ERREUR_SAISIE };
    for (size_t i = 0; i < sizeof codes / sizeof codes[0]; i++)
        VERIF(strcmp(interprete_message(codes[i]), "Erreur inconnue.") != 0 &&
              strcmp(interprete_message(codes[i]), "Erreur de contacts inconnue.") != 0);
    interprete_liberer(&it);
}

static void test_sauvegarde(void) {
    interprete_t it, autre;
    static unsigned char octets[1 << 16];
    interprete_initialiser(&it);
    it.sauvegarde = "test.sav";
    it.clef_defaut = "test_defaut.key";
    remove("test.sav");
    remove("test_defaut.key");
    VERIF(commande(&it, "load") == RSA_ERREUR_FICHIER);           /* ni sauvegarde ni clef par defaut */
    VERIF(commande(&it, "newkeys identifiant_tres_visible crypt") == CMD_SUCCES);
    VERIF(commande(&it, "newkeys signe sign") == CMD_SUCCES);
    FILE *formulaire = fopen("menu.txt", "w+");
    if (formulaire != NULL) {
        fputs("Martin\nBob\n\n", formulaire);
        rewind(formulaire);
        char l[] = "addcontact bob";
        VERIF(interprete_ligne(&it, l, formulaire, stdout) == CONTACTS_SUCCES);
        fclose(formulaire);
        printf("\n");
    }
    entree_clef_t avant = *table_chercher(&it.table, "identifiant_tres_visible");
    VERIF(commande(&it, "save") == CMD_SUCCES);
    VERIF(commande(&it, "save un deux") == CMD_ERREUR_SYNTAXE);

    /* la sauvegarde est chiffree (aucun identificateur en clair) et reservee a l'utilisateur */
    size_t taille = lire("test.sav", octets, sizeof octets);
    VERIF(taille > 16 && memcmp(octets, "DIYPG2", 6) == 0);
    int en_clair = 0;
    for (size_t i = 0; i + 24 <= taille; i++)
        if (memcmp(octets + i, "identifiant_tres_visible", 24) == 0) en_clair = 1;
    VERIF(!en_clair);
#ifndef _WIN32
    struct stat infos;
    VERIF(stat("test.sav", &infos) == 0 && (infos.st_mode & 0777) == 0600);
    VERIF(stat("test_defaut.key", &infos) == 0 && (infos.st_mode & 0777) == 0600);
#endif
    FILE *tmp = fopen("test.sav.tmp", "rb");
    VERIF(tmp == NULL);   /* le clair temporaire est supprime */
    if (tmp != NULL) fclose(tmp);

    /* load restaure tout, contacts compris */
    VERIF(commande(&it, "rmkeys identifiant_tres_visible") == CMD_SUCCES);
    VERIF(commande(&it, "rmcontact bob") == CONTACTS_SUCCES);
    VERIF(commande(&it, "load") == CMD_SUCCES);
    const entree_clef_t *apres = table_chercher(&it.table, "identifiant_tres_visible");
    VERIF(apres != NULL && memcmp(apres, &avant, sizeof avant) == 0);
    VERIF(it.table.nb == 2 && contacts_trouver(&it.contacts, "bob") != NULL);
    VERIF(it.table.contacts == &it.contacts);
    VERIF(commande(&it, "save copie.sav") == CMD_SUCCES && commande(&it, "load copie.sav") == CMD_SUCCES);

    /* sauvegarde abimee ou autre clef par defaut : refus, etat courant intact */
    VERIF(commande(&it, "newkeys en_plus crypt") == CMD_SUCCES);
    octets[taille / 2] ^= 0x40;
    VERIF(ecrire("abimee.sav", octets, taille));
    int r = commande(&it, "load abimee.sav");
    VERIF(r == CMD_ERREUR_FORMAT || r == RSA_ERREUR_CHIFFRE);
    VERIF(it.table.nb == 3 && table_chercher(&it.table, "en_plus") != NULL);
    VERIF(ecrire("coupee.sav", octets, taille - 5));
    VERIF(commande(&it, "load coupee.sav") != CMD_SUCCES && it.table.nb == 3);
    interprete_initialiser(&autre);
    autre.clef_defaut = "autre_defaut.key";
    remove("autre_defaut.key");
    VERIF(commande(&autre, "save autre.sav") == CMD_SUCCES);   /* cree une autre clef par defaut */
    VERIF(commande(&it, "load autre.sav") == CMD_ERREUR_FORMAT && it.table.nb == 3);
    VERIF(commande(&it, "load absente.sav") == RSA_ERREUR_FICHIER);
    VERIF(ecrire("test_defaut.key", (const unsigned char *)"pas une clef\n", 13));
    VERIF(commande(&it, "load") == CMD_ERREUR_FORMAT && it.table.nb == 3);
    interprete_liberer(&autre);
    interprete_liberer(&it);
}

/* Boucle de lecture : rejeu d'un petit script, comptage des erreurs, arret sur quit. */
static void test_boucle(void) {
    interprete_t it;
    interprete_initialiser(&it);
    FILE *script = fopen("script.txt", "w+"), *sortie = fopen("boucle.txt", "w+");
    VERIF(script != NULL && sortie != NULL);
    if (script == NULL || sortie == NULL) return;
    fputs("newkeys k crypt\nlistkeys\ninconnue\nnewkeys k sign\n\nquit\nlistkeys\n", script);
    rewind(script);
    VERIF(interprete_boucle(&it, script, sortie, 1, 0) == 2);
    rewind(sortie);
    size_t n = fread(texte, 1, sizeof texte - 1, sortie);
    texte[n] = '\0';
    VERIF(strcmp(texte, "> newkeys k crypt\n> listkeys\nk (crypt)\n> inconnue\n"
                        "Erreur : Commande inconnue (voir help).\n> newkeys k sign\n"
                        "Erreur : Cet identificateur existe deja.\n> quit\n") == 0);
    fclose(script);
    fclose(sortie);
    interprete_liberer(&it);
}

int main(void) {
    initialiser_console_utf8();
    init_log();
    test_table();
    test_commandes();
    test_sauvegarde();
    test_boucle();
    base64_cleanup();
    close_log();
    printf("Interprete phase 3 : %u verifications, %u echecs.\n", controles, echecs);
    return echecs == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
