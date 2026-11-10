/// \file interprete.c
/// \author Nada ARABA
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>
#include "interprete.h"
#include "../phase1_adam/chiffrement.h"      /* codes RSA_* */
#include "../phase1_loann/base64.h"          /* file_to_base64, base64_to_file (bin-2b64, b64-2bin) */
#include "../phase2_nada/gmp_rsa.h"          /* gen_keys_gmp */
#include "../phase2_adam/chiffrement_blocs.h"   /* chiffrement de la sauvegarde */
#include "../phase3_nada/commandes_nada.h"   /* crypt, uncrypt, show, savepub */

static const uint8_t signature_sauvegarde[8] = {'D', 'Y', 'S', 'A', 'V', 'E', '0', '1'};

void interprete_initialiser(interprete_t *it) {
    if (it == NULL) return;
    contacts_initialiser(&it->contacts);
    table_initialiser(&it->table, &it->contacts);
    it->sauvegarde = INTERPRETE_SAUVEGARDE;
    it->clef_defaut = INTERPRETE_CLEF_DEFAUT;
}

void interprete_liberer(interprete_t *it) {
    if (it == NULL) return;
    table_liberer(&it->table);
    contacts_liberer(&it->contacts);
}

/* ---------- commandes V1.1 ---------- */

static int cmd_listkeys(interprete_t *it, int argc, char **argv, FILE *sortie) {
    if (argc > 2) return CMD_ERREUR_SYNTAXE;
    if (argc == 2) {
        const entree_clef_t *e = table_chercher(&it->table, argv[1]);
        if (e == NULL) return CMD_ERREUR_ID_INCONNU;
        fprintf(sortie, "%s (%s)\n", e->id, e->type == CLEF_SIGN ? "sign" : "crypt");
        return CMD_SUCCES;
    }
    if (it->table.nb == 0) fprintf(sortie, "Aucune clef.\n");
    for (size_t i = 0; i < it->table.nb; i++)   /* identificateur et type, pas les clefs elles-memes */
        fprintf(sortie, "%s (%s)\n", it->table.entrees[i].id,
                it->table.entrees[i].type == CLEF_SIGN ? "sign" : "crypt");
    return ferror(sortie) ? RSA_ERREUR_FICHIER : CMD_SUCCES;
}

static int cmd_rmkeys(interprete_t *it, int argc, char **argv) {
    if (argc != 2) return CMD_ERREUR_SYNTAXE;
    return table_supprimer(&it->table, argv[1]);
}

static int cmd_newkeys(interprete_t *it, int argc, char **argv) {
    entree_clef_t e;
    if (argc != 3) return CMD_ERREUR_SYNTAXE;
    if (!table_identifiant_valide(argv[1])) return CMD_ERREUR_SYNTAXE;
    if (strcmp(argv[2], "crypt") != 0 && strcmp(argv[2], "sign") != 0) return CMD_ERREUR_TYPE;
    if (table_chercher(&it->table, argv[1]) != NULL) return CMD_ERREUR_DOUBLON;
    memset(&e, 0, sizeof e);
    strcpy(e.id, argv[1]);
    e.type = strcmp(argv[2], "sign") == 0 ? CLEF_SIGN : CLEF_CRYPT;
    e.a_priv = true;
    /* Clefs GMP (N > 2^32) : le chiffrement par blocs de 4 octets refuse les clefs de la phase 1. */
    int res = gen_keys_gmp(&e.pub, &e.priv, RSA_BITS_PREMIER_MAX);
    if (res != RSA_SUCCES) return res;
    return table_ajouter(&it->table, &e);
}

static int cmd_base64(int argc, char **argv, int vers_base64) {
    if (argc != 3) return CMD_ERREUR_SYNTAXE;
    int res = vers_base64 ? file_to_base64(argv[1], argv[2]) : base64_to_file(argv[1], argv[2]);
    return res == 0 ? CMD_SUCCES : RSA_ERREUR_FICHIER;
}

/* ---------- save / load : fichier chiffre avec une clef par defaut (interprete.pdf) ---------- */

/* Fichier texte : "DIYPG-CLEF-DEFAUT 1", puis E N D en hexadecimal. Cree au premier save. */
static int lire_clef_defaut(const char *chemin, rsaKey_t *pub, rsaKey_t *priv) {
    FILE *f = fopen(chemin, "r");
    char entete[32];
    uint64_t e, n, d;
    if (f == NULL) return RSA_ERREUR_FICHIER;
    int ok = fscanf(f, "%31s 1 %" SCNx64 " %" SCNx64 " %" SCNx64, entete, &e, &n, &d) == 4 &&
             strcmp(entete, "DIYPG-CLEF-DEFAUT") == 0 && n > UINT32_MAX && e > 0 && e < n && d > 0 && d < n;
    fclose(f);
    if (!ok) return CMD_ERREUR_FORMAT;
    *pub = (rsaKey_t){ e, n };
    *priv = (rsaKey_t){ d, n };
    return CMD_SUCCES;
}

static int creer_clef_defaut(const char *chemin, rsaKey_t *pub, rsaKey_t *priv) {
    int res = gen_keys_gmp(pub, priv, RSA_BITS_PREMIER_MAX);
    if (res != RSA_SUCCES) return res;
    FILE *f = fopen(chemin, "w");
    if (f == NULL) return RSA_ERREUR_FICHIER;
    int ok = fprintf(f, "DIYPG-CLEF-DEFAUT 1\n%" PRIx64 " %" PRIx64 " %" PRIx64 "\n",
                     pub->E, pub->N, priv->E) >= 0;
    if (fclose(f) != 0) ok = 0;
    /* contient la clef privee : lecture et ecriture reservees a l'utilisateur */
    if (!ok || chmod(chemin, S_IRUSR | S_IWUSR) != 0) return RSA_ERREUR_FICHIER;
    return CMD_SUCCES;
}

static void nom_temporaire(char *tampon, size_t taille, const char *chemin) {
    snprintf(tampon, taille, "%s.tmp", chemin);
}

static int code_contacts(int res) {
    if (res == CONTACTS_SUCCES) return CMD_SUCCES;
    if (res == CONTACTS_ERREUR_MEMOIRE) return CMD_ERREUR_MEMOIRE;
    if (res == CONTACTS_ERREUR_FICHIER) return RSA_ERREUR_FICHIER;
    return CMD_ERREUR_FORMAT;
}

static int sauver(interprete_t *it, const char *chemin) {
    rsaKey_t pub, priv;
    char temporaire[FILENAME_MAX];
    int res = lire_clef_defaut(it->clef_defaut, &pub, &priv);
    if (res == RSA_ERREUR_FICHIER) res = creer_clef_defaut(it->clef_defaut, &pub, &priv);
    if (res != CMD_SUCCES) return res;

    /* clair dans un temporaire (droits 0600), chiffre vers la destination, puis supprime */
    nom_temporaire(temporaire, sizeof temporaire, chemin);
    FILE *f = fopen(temporaire, "wb");
    if (f == NULL) return RSA_ERREUR_FICHIER;
    (void)chmod(temporaire, S_IRUSR | S_IWUSR);   /* best effort : le fichier est supprime juste apres */
    res = fwrite(signature_sauvegarde, 1, sizeof signature_sauvegarde, f) == sizeof signature_sauvegarde ?
          table_sauver(f, &it->table) : RSA_ERREUR_FICHIER;
    if (res == CMD_SUCCES) res = code_contacts(contacts_sauver(f, &it->contacts));
    if (fclose(f) != 0 && res == CMD_SUCCES) res = RSA_ERREUR_FICHIER;
    if (res == CMD_SUCCES) res = chiffrer_fichier_blocs_gmp(temporaire, chemin, &pub);
    remove(temporaire);
    if (res == RSA_SUCCES && chmod(chemin, S_IRUSR | S_IWUSR) != 0) res = RSA_ERREUR_FICHIER;
    return res;
}

static int charger(interprete_t *it, const char *chemin) {
    rsaKey_t pub, priv;
    char temporaire[FILENAME_MAX];
    table_clefs_t table;
    annuaire_contacts_t contacts;
    uint8_t signature[8];
    int res = lire_clef_defaut(it->clef_defaut, &pub, &priv);
    if (res != CMD_SUCCES) return res;

    nom_temporaire(temporaire, sizeof temporaire, chemin);
    res = dechiffrer_fichier_blocs_gmp(chemin, temporaire, &priv);
    if (res == RSA_ERREUR_CHIFFRE) return CMD_ERREUR_FORMAT;   /* fichier abime ou autre clef par defaut */
    if (res != RSA_SUCCES) return res;
    (void)chmod(temporaire, S_IRUSR | S_IWUSR);

    /* tout est lu dans des structures temporaires : l'etat courant n'est remplace qu'en cas de succes */
    table_initialiser(&table, NULL);
    contacts_initialiser(&contacts);
    FILE *f = fopen(temporaire, "rb");
    if (f == NULL) res = RSA_ERREUR_FICHIER;
    else {
        if (fread(signature, 1, sizeof signature, f) != sizeof signature ||
            memcmp(signature, signature_sauvegarde, sizeof signature) != 0) res = CMD_ERREUR_FORMAT;
        if (res == RSA_SUCCES) res = table_charger(f, &table);
        if (res == CMD_SUCCES) res = code_contacts(contacts_charger(f, &contacts));
        if (res == CMD_SUCCES && fgetc(f) != EOF) res = CMD_ERREUR_FORMAT;   /* donnees en trop */
        fclose(f);
    }
    remove(temporaire);
    if (res != CMD_SUCCES) {
        table_liberer(&table);
        contacts_liberer(&contacts);
        return res;
    }
    table_liberer(&it->table);
    contacts_liberer(&it->contacts);
    it->contacts = contacts;
    it->table = table;
    it->table.contacts = &it->contacts;
    return CMD_SUCCES;
}

static int cmd_save_load(interprete_t *it, int argc, char **argv, int sauvegarde) {
    if (argc > 2) return CMD_ERREUR_SYNTAXE;
    const char *chemin = argc == 2 ? argv[1] : it->sauvegarde;
    return sauvegarde ? sauver(it, chemin) : charger(it, chemin);
}

static void aide(FILE *sortie) {
    fputs("Commandes :\n"
          "  quit                                  quitter\n"
          "  help                                  cette aide\n"
          "  listkeys [<keyid>]                    identificateurs et types des clefs\n"
          "  newkeys <keyid> <crypt|sign>          nouvelle paire de clefs\n"
          "  rmkeys <keyid> | <contact/clef>       supprimer une clef\n"
          "  show <keyid> [pub] [priv]             afficher une clef en hexadecimal\n"
          "  crypt <fileIn> <fileOut> <keyid>      chiffrer un fichier (clef crypt)\n"
          "  uncrypt <fileIn> <fileOut> <keyid>    dechiffrer un fichier (clef crypt)\n"
          "  bin-2b64 <filein> <fileout>           binaire vers base 64\n"
          "  b64-2bin <filein> <fileout>           base 64 vers binaire\n"
          "  savepub <keyid> <file>                exporter une clef publique en base 64\n"
          "  save [<fileout>]                      sauvegarder (fichier chiffre)\n"
          "  load [<filein>]                       charger une sauvegarde\n"
          "  listcontacts [<idcontact>] [<nom>]    contacts et leurs clefs\n"
          "  addcontact <id> | modifycontact <id> | rmcontact <id>\n"
          "  addkeys <id ou nom>                   ajouter une clef publique a un contact\n"
          "<keyid> : clef de l'utilisateur, contact/clef, ou contact s'il n'a qu'une clef.\n", sortie);
}

/* ---------- aiguillage ---------- */

int interprete_executer(interprete_t *it, int argc, char **argv, FILE *entree, FILE *sortie) {
    if (it == NULL || argc < 1 || argv == NULL || argv[0] == NULL || entree == NULL || sortie == NULL)
        return RSA_ERREUR_PARAMETRE;
    for (int i = 0; i < argc; i++)
        if (argv[i] == NULL) return RSA_ERREUR_PARAMETRE;

    /* V2 d'abord : le module d'Adam renvoie CONTACTS_COMMANDE_INCONNUE (1) si la commande n'est pas pour lui */
    int res = contacts_executer(&it->contacts, argc, (const char *const *)argv, entree, sortie);
    if (res != CONTACTS_COMMANDE_INCONNUE) return res;

    const char *nom = argv[0];
    if (strcmp(nom, "quit") == 0) return argc == 1 ? INTERPRETE_QUITTER : CMD_ERREUR_SYNTAXE;
    if (strcmp(nom, "help") == 0) { aide(sortie); return CMD_SUCCES; }
    if (strcmp(nom, "listkeys") == 0) return cmd_listkeys(it, argc, argv, sortie);
    if (strcmp(nom, "rmkeys") == 0) return cmd_rmkeys(it, argc, argv);
    if (strcmp(nom, "newkeys") == 0) return cmd_newkeys(it, argc, argv);
    if (strcmp(nom, "bin-2b64") == 0) return cmd_base64(argc, argv, 1);
    if (strcmp(nom, "b64-2bin") == 0) return cmd_base64(argc, argv, 0);
    if (strcmp(nom, "save") == 0) return cmd_save_load(it, argc, argv, 1);
    if (strcmp(nom, "load") == 0) return cmd_save_load(it, argc, argv, 0);

    static const struct { const char *nom; commande_t commande; } commandes[] = {
        { "crypt", cmd_crypt }, { "uncrypt", cmd_uncrypt }, { "show", cmd_show }, { "savepub", cmd_savepub }
    };
    for (size_t i = 0; i < sizeof commandes / sizeof commandes[0]; i++) {
        if (strcmp(nom, commandes[i].nom) == 0) {
            fflush(sortie);   /* show ecrit sur stdout : garder l'ordre des messages */
            return commandes[i].commande(&it->table, argc, argv);
        }
    }
    return CMD_ERREUR_INCONNUE;
}

int interprete_ligne(interprete_t *it, char *ligne, FILE *entree, FILE *sortie) {
    char *argv[INTERPRETE_ARGS_MAX];
    int argc = 0;
    if (ligne == NULL) return RSA_ERREUR_PARAMETRE;
    for (char *mot = strtok(ligne, " \t\r\n"); mot != NULL; mot = strtok(NULL, " \t\r\n")) {
        if (argc == 0 && mot[0] == '#') return CMD_SUCCES;   /* commentaire */
        if (argc == INTERPRETE_ARGS_MAX) return CMD_ERREUR_SYNTAXE;
        argv[argc++] = mot;
    }
    if (argc == 0) return CMD_SUCCES;
    return interprete_executer(it, argc, argv, entree, sortie);
}

const char *interprete_message(int code) {
    if (code <= CONTACTS_ERREUR_PARAMETRE) return contacts_message_erreur(code);
    switch (code) {
    case CMD_SUCCES: return "Succes.";
    case RSA_ERREUR_PARAMETRE: return "Parametre invalide.";
    case RSA_ERREUR_CLE: return "Clef inadaptee (N doit depasser 2^32 - 1 pour les blocs de 4 octets).";
    case RSA_ERREUR_TAILLE: return "Taille invalide.";
    case RSA_ERREUR_CHIFFRE: return "Fichier chiffre invalide ou mauvaise clef.";
    case RSA_ERREUR_FICHIER: return "Fichier absent, illisible ou impossible a ecrire.";
    case CMD_ERREUR_SYNTAXE: return "Syntaxe incorrecte (voir help).";
    case CMD_ERREUR_ID_INCONNU: return "Identificateur inconnu.";
    case CMD_ERREUR_TYPE: return "Type de clef incorrect pour cette commande (crypt ou sign).";
    case CMD_ERREUR_PAS_DE_PRIVEE: return "Pas de clef privee pour cet identificateur.";
    case CMD_ERREUR_AMBIGU: return "Ce contact a plusieurs clefs : preciser contact/clef.";
    case CMD_ERREUR_DOUBLON: return "Cet identificateur existe deja.";
    case CMD_ERREUR_MEMOIRE: return "Memoire insuffisante.";
    case CMD_ERREUR_FORMAT: return "Sauvegarde invalide ou clef par defaut differente.";
    case CMD_ERREUR_INCONNUE: return "Commande inconnue (voir help).";
    default: return "Erreur inconnue.";
    }
}

int interprete_boucle(interprete_t *it, FILE *entree, FILE *sortie, int echo, int invite) {
    char ligne[1024];
    int erreurs = 0;
    if (it == NULL || entree == NULL || sortie == NULL) return 1;
    for (;;) {
        if (invite) { fputs("diypg> ", sortie); fflush(sortie); }
        if (fgets(ligne, sizeof ligne, entree) == NULL) break;
        int res;
        if (strchr(ligne, '\n') == NULL && !feof(entree)) {   /* ligne trop longue : on saute la suite */
            int c;
            while ((c = fgetc(entree)) != EOF && c != '\n') {}
            res = CMD_ERREUR_SYNTAXE;
        } else {
            if (echo && ligne[strspn(ligne, " \t\r\n")] != '\0')   /* rejeu : on recopie la commande */
                fprintf(sortie, "> %s%s", ligne, strchr(ligne, '\n') == NULL ? "\n" : "");
            res = interprete_ligne(it, ligne, entree, sortie);
        }
        fflush(stdout);
        if (res == INTERPRETE_QUITTER) break;
        if (res < 0) {
            fprintf(sortie, "Erreur : %s\n", interprete_message(res));
            erreurs++;
        }
        fflush(sortie);
    }
    return erreurs;
}
