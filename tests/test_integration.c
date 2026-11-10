/// \file test_integration.c
/// \author Nada ARABA
/// \brief Tests d'integration des phases 1 a 3 : deux utilisateurs echangent des fichiers chiffres
///        en passant par les modules de chacun, comme le demande le sujet (echange de clefs publiques
///        et de pieces jointes chiffrees en Base64).
///
/// Modules traverses : clefs GMP (Nada, phase 2), fichiers par blocs (Adam, phase 2),
/// Base64 et clefs publiques (Loann, phases 1 et 2), commandes crypt / uncrypt / savepub (Nada, phase 3),
/// contacts et leur sauvegarde (Adam, phase 3).
/// Le script de commandes de l'interprete complet est rejoue a part (phase3_interprete/tests/scenario.txt).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../phase1_nada/phase1/phase1.h"
#include "../phase1_adam/chiffrement.h"
#include "../phase1_loann/base64.h"
#include "../phase2_nada/gmp_rsa.h"
#include "../phase2_loann/base64_v2.h"
#include "../phase3_nada/commandes_nada.h"
#include "../phase3_adam/contacts.h"
#include "../console_utf8.h"

static unsigned controles = 0, echecs = 0;
#define VERIF(cond) do { ++controles; \
    if (!(cond)) { fprintf(stderr, "ECHEC integration ligne %d : %s\n", __LINE__, #cond); ++echecs; } } while (0)

/* Un utilisateur : ses clefs (table de l'interprete) et son annuaire de contacts. */
typedef struct {
    entree_clef_t clefs[4];
    table_clefs_t table;
    annuaire_contacts_t contacts;
} utilisateur_t;

static void utilisateur_init(utilisateur_t *u) {
    memset(u, 0, sizeof *u);
    contacts_initialiser(&u->contacts);
    u->table = (table_clefs_t){ u->clefs, 0, 4, &u->contacts };
}

/* Equivalent de "newkeys <id> <type>" sans passer par l'interprete : clefs GMP. */
static int newkeys(utilisateur_t *u, const char *id, type_clef_t type, unsigned bits) {
    entree_clef_t *e = &u->clefs[u->table.nb];
    if (u->table.nb == u->table.capacite || gen_keys_gmp(&e->pub, &e->priv, bits) != RSA_SUCCES) return 0;
    snprintf(e->id, sizeof e->id, "%s", id);
    e->type = type;
    e->a_priv = true;
    u->table.nb++;
    return 1;
}

/* Recoit un fichier de savepub et range la clef dans un contact (equivalent de addkeys). */
static int recevoir_clef(utilisateur_t *u, const char *fichier, const char *contact, const char *id_cle) {
    rsaKey_t cle;
    char type[PUBKEY_TYPE_TAILLE];
    if (import_pubkey_b64(fichier, &cle, type) != 0) return CONTACTS_ERREUR_FORMAT;
    if (contacts_trouver(&u->contacts, contact) == NULL &&
        contacts_ajouter(&u->contacts, contact, "", "", "") != CONTACTS_SUCCES) return CONTACTS_ERREUR_PARAMETRE;
    return contacts_ajouter_cle(&u->contacts, contact, id_cle,
                               strcmp(type, "sign") == 0 ? CONTACT_SIGN : CONTACT_CRYPT, &cle);
}

/* Appelle une commande de l'interprete : cmd(table, "nom", a1, a2, a3). */
static int executer(commande_t cmd, utilisateur_t *u, const char *nom,
                    const char *a1, const char *a2, const char *a3) {
    char *argv[4] = { (char *)nom, (char *)a1, (char *)a2, (char *)a3 };
    int argc = a3 != NULL ? 4 : 3;
    return cmd(&u->table, argc, argv);
}

static int ecrire_fichier(const char *chemin, const uint8_t *donnees, size_t taille) {
    FILE *f = fopen(chemin, "wb");
    if (f == NULL) return 0;
    int ok = fwrite(donnees, 1, taille, f) == taille;
    return fclose(f) == 0 && ok;
}

static int fichiers_egaux(const char *a, const char *b) {
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    int egaux = fa != NULL && fb != NULL, ca, cb;
    while (egaux) {
        ca = fgetc(fa);
        cb = fgetc(fb);
        if (ca != cb) egaux = 0;
        if (ca == EOF || cb == EOF) break;
    }
    if (fa != NULL) fclose(fa);
    if (fb != NULL) fclose(fb);
    return egaux;
}

/* Bob envoie un fichier a Alice : crypt avec la clef du contact, Base64 pour le mail,
 * puis Alice repasse en binaire et dechiffre. Renvoie 1 si Alice retrouve exactement le fichier. */
static int envoyer(utilisateur_t *expediteur, const char *ref_destinataire,
                   utilisateur_t *destinataire, const char *clef_destinataire, const char *clair) {
    return executer(cmd_crypt, expediteur, "crypt", clair, "envoi.bin", ref_destinataire) == RSA_SUCCES &&
           file_to_base64_v2("envoi.bin", "piece_jointe.b64") == 0 &&                       /* bin-2b64 */
           base64_to_file_v2("piece_jointe.b64", "recu.bin") == 0 &&                       /* b64-2bin */
           executer(cmd_uncrypt, destinataire, "uncrypt", "recu.bin", "recu.txt", clef_destinataire) == RSA_SUCCES &&
           fichiers_egaux(clair, "recu.txt");
}

static void test_echange(utilisateur_t *alice, utilisateur_t *bob, utilisateur_t *charlie) {
    /* 1. Chacun cree ses clefs et publie sa clef publique avec savepub. */
    VERIF(newkeys(alice, "perso", CLEF_CRYPT, 28) && newkeys(alice, "signature", CLEF_SIGN, 28));
    VERIF(newkeys(bob, "bob", CLEF_CRYPT, 28));
    VERIF(newkeys(charlie, "charlie", CLEF_CRYPT, 28));
    VERIF(executer(cmd_savepub, alice, "savepub", "perso", "alice_perso.b64", NULL) == CMD_SUCCES);
    VERIF(executer(cmd_savepub, alice, "savepub", "signature", "alice_signature.b64", NULL) == CMD_SUCCES);
    VERIF(executer(cmd_savepub, bob, "savepub", "bob", "bob.b64", NULL) == CMD_SUCCES);

    /* 2. Bob range la clef d'Alice dans son contact "alice" ; Alice range celle de Bob. */
    VERIF(recevoir_clef(bob, "alice_perso.b64", "alice", "perso") == CONTACTS_SUCCES);
    VERIF(recevoir_clef(alice, "bob.b64", "bob", "travail") == CONTACTS_SUCCES);

    /* 3. Echanges dans les deux sens, tailles variees (cahier des charges : 0, 1, 3, 4, 5, 1000, binaire). */
    static const size_t tailles[] = { 0, 1, 3, 4, 5, 1000, 4099, 100003 };
    static uint8_t donnees[100003];
    for (size_t i = 0; i < sizeof donnees; i++) donnees[i] = (uint8_t)(i * 31u + (i >> 8));
    for (size_t i = 0; i < sizeof tailles / sizeof tailles[0]; i++) {
        VERIF(ecrire_fichier("message.bin", donnees, tailles[i]));
        VERIF(envoyer(bob, "alice", alice, "perso", "message.bin"));          /* contact seul (une clef) */
        VERIF(envoyer(alice, "bob/travail", bob, "bob", "message.bin"));      /* contact/clef */
    }
    const char texte[] = "Rendez-vous à 14h, salle « Turing » — été 2026 ✓\n";
    VERIF(ecrire_fichier("message.txt", (const uint8_t *)texte, sizeof texte - 1));   /* UTF-8 */
    VERIF(envoyer(bob, "alice/perso", alice, "perso", "message.txt"));

    /* 4. Un tiers ne peut pas lire un message qui ne lui est pas destine. */
    VERIF(executer(cmd_crypt, bob, "crypt", "message.txt", "pour_alice.bin", "alice") == RSA_SUCCES);
    int r = executer(cmd_uncrypt, charlie, "uncrypt", "pour_alice.bin", "lu_par_charlie.txt", "charlie");
    VERIF(r != RSA_SUCCES || !fichiers_egaux("message.txt", "lu_par_charlie.txt"));

    /* 5. Piece jointe abimee pendant l'envoi : jamais le bon clair en sortie. */
    VERIF(file_to_base64_v2("pour_alice.bin", "abimee.b64") == 0);
    FILE *f = fopen("abimee.b64", "r+b");
    VERIF(f != NULL);
    if (f != NULL) {
        int c;
        VERIF(fseek(f, 40, SEEK_SET) == 0 && (c = fgetc(f)) != EOF);
        VERIF(fseek(f, 40, SEEK_SET) == 0 && fputc(c == 'A' ? 'B' : 'A', f) != EOF);
        VERIF(fclose(f) == 0);
    }
    if (base64_to_file_v2("abimee.b64", "abimee.bin") == 0) {
        r = executer(cmd_uncrypt, alice, "uncrypt", "abimee.bin", "abimee.txt", "perso");
        VERIF(r != RSA_SUCCES || !fichiers_egaux("message.txt", "abimee.txt"));
    }

    /* 6. Le type voyage avec la clef : une clef sign recue ne sert pas a chiffrer. */
    VERIF(recevoir_clef(bob, "alice_signature.b64", "alice", "signature") == CONTACTS_SUCCES);
    VERIF(executer(cmd_crypt, bob, "crypt", "message.txt", "x.bin", "alice/signature") == CMD_ERREUR_TYPE);
    VERIF(executer(cmd_crypt, bob, "crypt", "message.txt", "x.bin", "alice") == CMD_ERREUR_AMBIGU);   /* 2 clefs */
    VERIF(envoyer(bob, "alice/perso", alice, "perso", "message.txt"));
    VERIF(executer(cmd_uncrypt, bob, "uncrypt", "pour_alice.bin", "x.txt", "alice/perso") == CMD_ERREUR_PAS_DE_PRIVEE);
}

/* Les contacts sauvegardes puis recharges (section d'Adam pour save/load) servent toujours a chiffrer. */
static void test_sauvegarde_contacts(utilisateur_t *alice, utilisateur_t *bob) {
    FILE *f = fopen("contacts_bob.bin", "wb");
    VERIF(f != NULL);
    if (f == NULL) return;
    VERIF(contacts_sauver(f, &bob->contacts) == CONTACTS_SUCCES);
    VERIF(fclose(f) == 0);

    utilisateur_t bob_relance;   /* nouvelle session : clefs identiques, annuaire relu depuis le fichier */
    utilisateur_init(&bob_relance);
    bob_relance.clefs[0] = bob->clefs[0];
    bob_relance.table.nb = 1;
    f = fopen("contacts_bob.bin", "rb");
    VERIF(f != NULL);
    if (f != NULL) {
        VERIF(contacts_charger(f, &bob_relance.contacts) == CONTACTS_SUCCES);
        VERIF(fclose(f) == 0);
    }
    VERIF(contacts_nombre(&bob_relance.contacts) == contacts_nombre(&bob->contacts));
    VERIF(envoyer(&bob_relance, "alice/perso", alice, "perso", "message.txt"));
    VERIF(executer(cmd_crypt, &bob_relance, "crypt", "message.txt", "x.bin", "alice/signature") == CMD_ERREUR_TYPE);
    contacts_liberer(&bob_relance.contacts);
}

int main(void) {
    static utilisateur_t alice, bob, charlie;
    initialiser_console_utf8();
    init_log();
    utilisateur_init(&alice);
    utilisateur_init(&bob);
    utilisateur_init(&charlie);
    test_echange(&alice, &bob, &charlie);
    test_sauvegarde_contacts(&alice, &bob);
    contacts_liberer(&alice.contacts);
    contacts_liberer(&bob.contacts);
    contacts_liberer(&charlie.contacts);
    base64_cleanup();
    close_log();
    printf("Integration phases 1 a 3 : %u verifications, %u echecs.\n", controles, echecs);
    return echecs == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
