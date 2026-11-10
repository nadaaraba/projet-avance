#include "../contacts.h"
#include "../contacts_interpreteur.h"
#include "../../console_utf8.h"

static unsigned int controles = 0, echecs = 0;
#define VERIFIER(condition) do { \
    ++controles; \
    if (!(condition)) { fprintf(stderr, "Echec contacts ligne %d : %s\n", __LINE__, #condition); ++echecs; } \
} while (0)

static int sauver(const char *nom, const annuaire_contacts_t *annuaire)
{
    FILE *f = fopen(nom, "wb");
    int resultat;
    if (f == NULL) return CONTACTS_ERREUR_FICHIER;
    resultat = contacts_sauver(f, annuaire);
    if (fclose(f) != 0) resultat = CONTACTS_ERREUR_FICHIER;
    return resultat;
}
static int charger(const char *nom, annuaire_contacts_t *annuaire)
{
    FILE *f = fopen(nom, "rb");
    int resultat;
    if (f == NULL) return CONTACTS_ERREUR_FICHIER;
    resultat = contacts_charger(f, annuaire);
    if (fclose(f) != 0) resultat = CONTACTS_ERREUR_FICHIER;
    return resultat;
}
static size_t lire(const char *nom, unsigned char *octets, size_t capacite)
{
    FILE *f = fopen(nom, "rb");
    size_t taille;
    if (f == NULL) return 0;
    taille = fread(octets, 1, capacite, f);
    if (fclose(f) != 0) return 0;
    return taille;
}
static int ecrire(const char *nom, const unsigned char *octets, size_t taille)
{
    FILE *f = fopen(nom, "wb");
    int ok;
    if (f == NULL) return 0;
    ok = fwrite(octets, 1, taille, f) == taille;
    if (fclose(f) != 0) ok = 0;
    return ok;
}
static int commande(annuaire_contacts_t *annuaire, const char *nom, int argc,
                     const char *const argv[], const char *formulaire)
{
    FILE *entree, *sortie;
    int resultat;
    if (!ecrire("formulaire.txt", (const unsigned char *)formulaire, strlen(formulaire)))
        return CONTACTS_ERREUR_FICHIER;
    entree = fopen("formulaire.txt", "rb");
    sortie = fopen("affichage.txt", "wb");
    if (entree == NULL || sortie == NULL) {
        if (entree != NULL) fclose(entree);
        if (sortie != NULL) fclose(sortie);
        return CONTACTS_ERREUR_FICHIER;
    }
    /* contacts_executer recoit argv[0] = nom de la commande, comme main. */
    const char *argv_total[8] = {nom};
    for (int i = 0; i < argc && i < 7; ++i) argv_total[i + 1] = argv[i];
    resultat = contacts_executer(annuaire, argc + 1, argv_total, entree, sortie);
    if (fclose(entree) != 0) resultat = CONTACTS_ERREUR_FICHIER;
    if (fclose(sortie) != 0) resultat = CONTACTS_ERREUR_FICHIER;
    return resultat;
}

int main(void)
{
    annuaire_contacts_t a = {NULL}, b = {NULL}, garde = {NULL};
    rsaKey_t cle = {UINT64_C(65537), UINT64_C(18446743979220271189)};
    rsaKey_t signature = {17, 3233}, copie = {42, 43};
    contact_cle_t infos_cle;
    const contact_infos_t *infos;
    unsigned char donnees[8192], avant[8192], apres[8192], modifie[8192];
    size_t taille, taille_avant;
    char id_long[NAME_MAX_SIZE + 2], nom_long[CONTACT_TEXTE_TAILLE + 1];

    initialiser_console_utf8();
    contacts_initialiser(&a);
    VERIFIER(contacts_nombre(&a) == 0 && contacts_nombre(NULL) == 0);
    VERIFIER(contacts_ajouter(NULL, "a", "", "", "") == CONTACTS_ERREUR_PARAMETRE);
    VERIFIER(contacts_ajouter(&a, "", "", "", "") == CONTACTS_ERREUR_IDENTIFIANT);
    VERIFIER(contacts_ajouter(&a, "a/b", "", "", "") == CONTACTS_ERREUR_IDENTIFIANT);
    VERIFIER(contacts_ajouter(&a, "a b", "", "", "") == CONTACTS_ERREUR_IDENTIFIANT);
    memset(id_long, 'a', sizeof id_long - 1); id_long[sizeof id_long - 1] = '\0';
    VERIFIER(contacts_ajouter(&a, id_long, "", "", "") == CONTACTS_ERREUR_IDENTIFIANT);
    id_long[NAME_MAX_SIZE] = '\0';
    char reference_longue[2 * NAME_MAX_SIZE + 2];
    snprintf(reference_longue, sizeof reference_longue, "%s/%s", id_long, id_long);
    VERIFIER(contacts_ajouter(&a, id_long, "", "", "") == 0);
    VERIFIER(contacts_ajouter_cle(&a, id_long, id_long, CONTACT_CRYPT, &cle) == 0);
    VERIFIER(contacts_lire_cle(&a, reference_longue, &infos_cle) == 0 &&
             strcmp(infos_cle.identifiant, id_long) == 0);
    VERIFIER(sauver("limite.bin", &a) == 0);
    VERIFIER(charger("limite.bin", &b) == 0);
    VERIFIER(contacts_lire_cle(&b, reference_longue, &infos_cle) == 0);
    contacts_liberer(&b);
    VERIFIER(contacts_supprimer(&a, id_long) == 0);
    VERIFIER(contacts_ajouter(&a, "alice", "Évrard", "Chloé", "Équipe de TP") == 0);
    VERIFIER(contacts_ajouter(&a, "alice", "", "", "") == CONTACTS_ERREUR_DOUBLON);
    VERIFIER(contacts_ajouter(&a, "bob", "Martin", "Bob", "") == 0);
    VERIFIER(contacts_ajouter(&a, "charlie", "Martin", "Charlie", "") == 0);
    VERIFIER(contacts_nombre(&a) == 3);
    VERIFIER(contacts_ajouter_cle(&a, "Martin", "pro", CONTACT_CRYPT, &cle) == CONTACTS_ERREUR_AMBIGU);
    VERIFIER(contacts_ajouter_cle(&a, "Évrard", "pro", CONTACT_CRYPT, &cle) == 0);
    VERIFIER(contacts_ajouter_cle(&a, "alice", "pro", CONTACT_CRYPT, &cle) == CONTACTS_ERREUR_DOUBLON);
    VERIFIER(contacts_ajouter_cle(&a, "alice", "signature", CONTACT_SIGN, &signature) == 0);
    VERIFIER(contacts_ajouter_cle(&a, "bob", "pro", CONTACT_CRYPT, &cle) == 0);
    VERIFIER(contacts_ajouter_cle(&a, "absent", "pro", CONTACT_CRYPT, &cle) == CONTACTS_ERREUR_INTROUVABLE);
    VERIFIER(contacts_ajouter_cle(&a, "alice", "bad", (contact_type_cle_t)7, &cle) == CONTACTS_ERREUR_TYPE);
    VERIFIER(contacts_ajouter_cle(&a, "alice", "a/b", CONTACT_CRYPT, &cle) == CONTACTS_ERREUR_IDENTIFIANT);
    VERIFIER(contacts_ajouter_cle(&a, "alice", "bad", CONTACT_CRYPT,
                                 &(rsaKey_t){0, 9}) == CONTACTS_ERREUR_PARAMETRE);
    /* Une clef crypt doit servir aux blocs de 4 octets : N > 2^32 - 1. Pas de borne pour sign. */
    VERIFIER(contacts_ajouter_cle(&a, "alice", "petite", CONTACT_CRYPT,
                                 &(rsaKey_t){3, UINT32_MAX}) == CONTACTS_ERREUR_PARAMETRE);
    VERIFIER(contacts_lire_cle(&a, "alice/petite", &infos_cle) == CONTACTS_ERREUR_INTROUVABLE);
    VERIFIER(contacts_lire_cle(&a, "alice/pro", &infos_cle) == 0);
    VERIFIER(infos_cle.type == CONTACT_CRYPT && infos_cle.publique.N == cle.N);
    VERIFIER(contacts_resoudre(&a, "alice/signature", CONTACT_CRYPT, &copie) == CONTACTS_ERREUR_TYPE);
    VERIFIER(copie.E == 42 && copie.N == 43);
    VERIFIER(contacts_resoudre(&a, "alice/pro", CONTACT_CRYPT, &copie) == 0);
    VERIFIER(copie.E == cle.E && copie.N == cle.N);
    VERIFIER(contacts_resoudre(&a, "alice/signature", CONTACT_SIGN, &copie) == 0);
    VERIFIER(copie.E == 17 && copie.N == 3233);
    entree_clef_t entree = {.a_priv = true, .priv = {7, 9}};
    VERIFIER(contacts_resoudre_entree(&a, "alice/pro", &entree) == CMD_SUCCES);
    VERIFIER(strcmp(entree.id, "pro") == 0 && entree.type == CLEF_CRYPT);
    VERIFIER(entree.pub.E == cle.E && entree.pub.N == cle.N &&
             !entree.a_priv && entree.priv.E == 0 && entree.priv.N == 0);
    VERIFIER(contacts_resoudre_entree(&a, "alice/signature", &entree) == CMD_SUCCES);
    VERIFIER(entree.type == CLEF_SIGN && entree.pub.E == signature.E && entree.pub.N == signature.N);
    VERIFIER(contacts_resoudre_entree(&a, "alice/absente", &entree) == CMD_ERREUR_ID_INCONNU);
    VERIFIER(contacts_resoudre_entree(&a, "alice", &entree) == CMD_ERREUR_AMBIGU);   /* deux clefs */
    VERIFIER(contacts_resoudre_entree(&a, "a b", &entree) == CMD_ERREUR_SYNTAXE);
    VERIFIER(strcmp(entree.id, "signature") == 0 && entree.type == CLEF_SIGN);
    /* Sujet V2 : "contact" seul suffit si le contact a une seule clef. */
    VERIFIER(contacts_lire_cle(&a, "alice", &infos_cle) == CONTACTS_ERREUR_AMBIGU);
    VERIFIER(contacts_lire_cle(&a, "bob", &infos_cle) == 0 && strcmp(infos_cle.identifiant, "pro") == 0);
    VERIFIER(contacts_lire_cle(&a, "charlie", &infos_cle) == CONTACTS_ERREUR_INTROUVABLE);   /* aucune clef */
    VERIFIER(contacts_lire_cle(&a, "absent", &infos_cle) == CONTACTS_ERREUR_INTROUVABLE);
    VERIFIER(contacts_resoudre(&a, "bob", CONTACT_CRYPT, &copie) == 0 && copie.N == cle.N);
    VERIFIER(contacts_resoudre(&a, "bob", CONTACT_SIGN, &copie) == CONTACTS_ERREUR_TYPE);
    VERIFIER(contacts_supprimer_cle(&a, "bob") == CONTACTS_ERREUR_IDENTIFIANT);   /* rmkeys exige contact/clef */
    const char *invalides[] = {"a b", "/pro", "alice/", "alice/pro/autre", "a b/pro", "alice/a b"};
    for (size_t i = 0; i < sizeof invalides / sizeof invalides[0]; ++i)
        VERIFIER(contacts_lire_cle(&a, invalides[i], &infos_cle) == CONTACTS_ERREUR_IDENTIFIANT);
    VERIFIER(contacts_lire_cle(&a, "absent/pro", &infos_cle) == CONTACTS_ERREUR_INTROUVABLE);
    VERIFIER(contacts_lire_cle(&a, "alice/absente", &infos_cle) == CONTACTS_ERREUR_INTROUVABLE);
    VERIFIER(contacts_lire_cle(&a, NULL, &infos_cle) == CONTACTS_ERREUR_PARAMETRE);
    infos = contacts_trouver(&a, "alice");
    VERIFIER(infos != NULL && strcmp(infos->prenom, "Chloé") == 0);
    /* On peut reutiliser les valeurs du contact pour le modifier. */
    VERIFIER(contacts_modifier(&a, "alice", infos->nom, infos->prenom, infos->commentaire) == 0);
    memset(nom_long, 'n', sizeof nom_long - 1); nom_long[sizeof nom_long - 1] = '\0';
    VERIFIER(contacts_modifier(&a, "alice", nom_long, "", "") == CONTACTS_ERREUR_PARAMETRE);
    VERIFIER(strcmp(contacts_trouver(&a, "alice")->prenom, "Chloé") == 0);
    VERIFIER(contacts_modifier(&a, "alice", "Évrard", "Chloé", "modifié") == 0);
    VERIFIER(contacts_modifier(&a, "absent", "", "", "") == CONTACTS_ERREUR_INTROUVABLE);

    VERIFIER(sauver("contacts.bin", &a) == 0);
    VERIFIER(charger("contacts.bin", &b) == 0);
    VERIFIER(contacts_nombre(&b) == 3);
    VERIFIER(contacts_resoudre(&b, "alice/pro", CONTACT_CRYPT, &copie) == 0 && copie.N == cle.N);
    VERIFIER(strcmp(contacts_trouver(&b, "alice")->commentaire, "modifié") == 0);
    VERIFIER(sauver("contacts_copie.bin", &b) == 0);
    taille = lire("contacts.bin", donnees, sizeof donnees);
    VERIFIER(taille > 12 && lire("contacts_copie.bin", apres, sizeof apres) == taille);
    VERIFIER(memcmp(donnees, apres, taille) == 0);
    VERIFIER(contacts_ajouter(&garde, "garder", "Avant", "Test", "") == 0);
    VERIFIER(sauver("avant.bin", &garde) == 0);
    taille_avant = lire("avant.bin", avant, sizeof avant);
    /* Chaque coupure doit echouer sans remplacer l'annuaire de destination. */
    for (size_t i = 0; i < taille; ++i) {
        VERIFIER(ecrire("invalide.bin", donnees, i));
        VERIFIER(charger("invalide.bin", &garde) == CONTACTS_ERREUR_FORMAT);
        VERIFIER(contacts_nombre(&garde) == 1 && contacts_trouver(&garde, "garder") != NULL);
    }
    memcpy(modifie, donnees, taille); modifie[0] ^= 1;
    VERIFIER(ecrire("invalide.bin", modifie, taille));
    VERIFIER(charger("invalide.bin", &garde) == CONTACTS_ERREUR_FORMAT);
    memcpy(modifie, donnees, taille); modifie[12] = 255; modifie[13] = 255;
    VERIFIER(ecrire("invalide.bin", modifie, taille));
    VERIFIER(charger("invalide.bin", &garde) == CONTACTS_ERREUR_FORMAT);
    memcpy(modifie, donnees, taille); modifie[14] = 0;
    VERIFIER(ecrire("invalide.bin", modifie, taille));
    VERIFIER(charger("invalide.bin", &garde) == CONTACTS_ERREUR_FORMAT);
    VERIFIER(sauver("apres.bin", &garde) == 0);
    VERIFIER(lire("apres.bin", apres, sizeof apres) == taille_avant &&
             memcmp(avant, apres, taille_avant) == 0);
    FILE *f = fopen("contacts.bin", "rb");
    VERIFIER(f != NULL);
    if (f != NULL) {
        VERIFIER(contacts_sauver(f, &a) == CONTACTS_ERREUR_FICHIER);
        VERIFIER(fclose(f) == 0);
    }
    f = fopen("lecture_interdite.bin", "wb");
    VERIFIER(f != NULL);
    if (f != NULL) {
        VERIFIER(contacts_charger(f, &garde) == CONTACTS_ERREUR_FICHIER);
        VERIFIER(fclose(f) == 0);
    }
    VERIFIER(contacts_sauver(NULL, &a) == CONTACTS_ERREUR_PARAMETRE);
    VERIFIER(contacts_charger(NULL, &a) == CONTACTS_ERREUR_PARAMETRE);

    VERIFIER(contacts_supprimer_cle(&b, "alice/pro") == 0);
    VERIFIER(contacts_trouver(&b, "alice") != NULL);
    VERIFIER(contacts_lire_cle(&b, "alice/pro", &infos_cle) == CONTACTS_ERREUR_INTROUVABLE);
    VERIFIER(contacts_lire_cle(&b, "alice/signature", &infos_cle) == 0);
    VERIFIER(contacts_lire_cle(&b, "bob/pro", &infos_cle) == 0);
    VERIFIER(contacts_supprimer(&b, "alice") == 0 && contacts_trouver(&b, "alice") == NULL);
    VERIFIER(contacts_supprimer(&b, "alice") == CONTACTS_ERREUR_INTROUVABLE);
    VERIFIER(contacts_lire_cle(&b, "alice/signature", &infos_cle) == CONTACTS_ERREUR_INTROUVABLE);

    const char *dora[] = {"dora"}, *ref[] = {"dora/travail"};
    VERIFIER(commande(&b, "addcontact", 1, dora, "Durand\r\nÉlodie\r\nCollègue\r\n") == 0);
    VERIFIER(strcmp(contacts_trouver(&b, "dora")->prenom, "Élodie") == 0);
    VERIFIER(commande(&b, "addkeys", 1, dora, "travail\ncrypt\n10001\n100040003\n") == 0);
    VERIFIER(commande(&b, "show", 1, ref, "") == CONTACTS_COMMANDE_INCONNUE);
    VERIFIER(commande(&b, "modifycontact", 1, dora, "3\nChangé\n0\n") == 0);
    size_t taille_affichage = lire("affichage.txt", apres, sizeof apres - 1);
    apres[taille_affichage] = '\0';
    VERIFIER(strstr((const char *)apres, "Élodie Durand") != NULL);
    VERIFIER(strstr((const char *)apres, "Collègue") != NULL);
    VERIFIER(strcmp(contacts_trouver(&b, "dora")->commentaire, "Changé") == 0);
    VERIFIER(commande(&b, "modifycontact", 1, dora, "3\nSans validation\n") == CONTACTS_ERREUR_SAISIE);
    VERIFIER(strcmp(contacts_trouver(&b, "dora")->commentaire, "Changé") == 0);
    VERIFIER(commande(&b, "listcontacts", 0, NULL, "") == 0);
    VERIFIER(commande(&b, "listcontacts", 1, dora, "") == 0);
    VERIFIER(commande(&b, "rmkeys", 1, ref, "") == 0);
    VERIFIER(contacts_trouver(&b, "dora") != NULL);
    VERIFIER(commande(&b, "rmcontact", 1, dora, "") == 0);
    VERIFIER(commande(&b, "show", 1, dora, "") == CONTACTS_COMMANDE_INCONNUE);
    VERIFIER(commande(&b, "rmkeys", 1, dora, "") == CONTACTS_COMMANDE_INCONNUE);
    VERIFIER(commande(&b, "save", 0, NULL, "") == CONTACTS_COMMANDE_INCONNUE);
    VERIFIER(commande(&b, "crypt", 0, NULL, "") == CONTACTS_COMMANDE_INCONNUE);
    VERIFIER(commande(&b, "addcontact", 0, NULL, "") == CONTACTS_ERREUR_PARAMETRE);
    VERIFIER(commande(&b, "addcontact", 1, dora, "Seulement un nom\n") == CONTACTS_ERREUR_SAISIE);
    VERIFIER(contacts_trouver(&b, "dora") == NULL);

    /* Doublons et valeurs invalides dans la section binaire. */
    annuaire_contacts_t unique = {NULL};
    VERIFIER(contacts_ajouter(&unique, "x", "", "", "") == 0);
    VERIFIER(contacts_ajouter_cle(&unique, "x", "k", CONTACT_CRYPT, &cle) == 0);
    VERIFIER(sauver("unique.bin", &unique) == 0);
    size_t taille_unique = lire("unique.bin", donnees, sizeof donnees);
    VERIFIER(taille_unique == 45);
    memcpy(modifie, donnees, taille_unique);
    modifie[21] = 2;
    memcpy(modifie + taille_unique, donnees + 25, 20);
    VERIFIER(ecrire("invalide.bin", modifie, taille_unique + 20));
    VERIFIER(charger("invalide.bin", &garde) == CONTACTS_ERREUR_FORMAT);
    for (size_t cas = 0; cas < 3; ++cas) {
        memcpy(modifie, donnees, taille_unique);
        if (cas == 0) modifie[28] = 9;
        else memset(modifie + (cas == 1 ? 29 : 37), 0, 8);
        VERIFIER(ecrire("invalide.bin", modifie, taille_unique));
        VERIFIER(charger("invalide.bin", &garde) == CONTACTS_ERREUR_FORMAT);
    }
    memcpy(modifie, avant, taille_avant);
    modifie[8] = 2;
    memcpy(modifie + taille_avant, avant + 12, taille_avant - 12);
    VERIFIER(ecrire("invalide.bin", modifie, 2 * taille_avant - 12));
    VERIFIER(charger("invalide.bin", &garde) == CONTACTS_ERREUR_FORMAT);
    VERIFIER(contacts_trouver(&garde, "garder") != NULL);

    /* Une section peut etre suivie d'une autre partie du save/load global. */
    f = fopen("sections.bin", "wb+");
    VERIFIER(f != NULL);
    if (f != NULL) {
        VERIFIER(contacts_sauver(f, &a) == 0);
        VERIFIER(fwrite("SUITE", 1, 5, f) == 5);
        VERIFIER(fseek(f, 0, SEEK_SET) == 0);
        VERIFIER(contacts_charger(f, &b) == 0);
        VERIFIER(fread(apres, 1, 5, f) == 5 && memcmp(apres, "SUITE", 5) == 0);
        VERIFIER(fclose(f) == 0);
    }
    const char *absent[] = {"absent"};
    VERIFIER(commande(&b, "addkeys", 1, absent, "") == CONTACTS_ERREUR_INTROUVABLE);
    VERIFIER(commande(&b, "addkeys", 1, dora, "") == CONTACTS_ERREUR_INTROUVABLE);
    const char *alice[] = {"alice"};
    VERIFIER(commande(&b, "addkeys", 1, alice, "bad\ncrypt\n10000000000000000\n") == CONTACTS_ERREUR_SAISIE);
    VERIFIER(commande(&b, "addkeys", 1, alice, "bad\ncrypt\n-1\n") == CONTACTS_ERREUR_SAISIE);
    VERIFIER(commande(&b, "modifycontact", 1, alice, "9\n") == CONTACTS_ERREUR_SAISIE);
    char formulaire_long[700];
    memset(formulaire_long, 'n', sizeof formulaire_long - 2);
    formulaire_long[sizeof formulaire_long - 2] = '\n';
    formulaire_long[sizeof formulaire_long - 1] = '\0';
    VERIFIER(commande(&b, "addcontact", 1, dora, formulaire_long) == CONTACTS_ERREUR_SAISIE);
    VERIFIER(contacts_trouver(&b, "dora") == NULL);
    contacts_liberer(&unique);

    /* Fichier de reference compare aussi par le lanceur Python. */
    VERIFIER(sauver("contacts_reference.bin", &a) == 0);
    contacts_liberer(&a); contacts_liberer(&b); contacts_liberer(&garde);
    contacts_liberer(&a);
    VERIFIER(contacts_nombre(&a) == 0 && contacts_trouver(&a, "alice") == NULL);
    printf("Contacts V2 : %u verifications, %u echecs.\n", controles, echecs);
    return echecs ? EXIT_FAILURE : EXIT_SUCCESS;
}
