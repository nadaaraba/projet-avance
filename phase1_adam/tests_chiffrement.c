#include "chiffrement.h"
#include "tests_chiffrement.h"

typedef struct {
    unsigned int verifications;
    unsigned int echecs;
} bilan_t;

#define VERIFIER(bilan, condition) \
    do { \
        ++(bilan)->verifications; \
        if (!(condition)) { \
            ++(bilan)->echecs; \
            fprintf(stderr, "Echec ligne %d : %s\n", __LINE__, #condition); \
        } \
    } while (0)

static const rsaKey_t cle_publique = {17, 3233};
static const rsaKey_t cle_privee = {2753, 3233};

static void tester_octets(bilan_t *bilan)
{
    const uint64_t premiers[] = {
        17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79
    };
    const size_t nombre = sizeof premiers / sizeof premiers[0];
    size_t i;
    size_t j;

    /* 120 paires distinctes ; chaque paire est testee sur les 256 octets. */
    for (i = 0; i < nombre; ++i) {
        for (j = i + 1; j < nombre; ++j) {
            uint64_t phi = (premiers[i] - 1) * (premiers[j] - 1);
            uint64_t d;
            unsigned int valeur;
            rsaKey_t publique = {17, premiers[i] * premiers[j]};
            rsaKey_t privee;

            /* Inverse de 17, obtenu independamment des outils RSA fournis. */
            for (d = 1; d < phi && (17 * d) % phi != 1; ++d) {
            }
            VERIFIER(bilan, d < phi);
            if (d == phi) {
                continue;
            }
            privee.E = d;
            privee.N = publique.N;
            for (valeur = 0; valeur <= UINT8_MAX; ++valeur) {
                uint64_t chiffre = 0;
                uint8_t clair = 0;
                VERIFIER(bilan, chiffrer_octet((uint8_t)valeur, &publique,
                                              &chiffre) == RSA_SUCCES);
                VERIFIER(bilan, chiffre < publique.N);
                VERIFIER(bilan, dechiffrer_octet(chiffre, &privee,
                                                &clair) == RSA_SUCCES);
                VERIFIER(bilan, clair == valeur);
            }
        }
    }
    {
        uint64_t chiffre = 0;
        VERIFIER(bilan, chiffrer_octet(65, &cle_publique, &chiffre) == RSA_SUCCES);
        VERIFIER(bilan, chiffre == 2790);
    }
}

static void tester_tableaux(bilan_t *bilan)
{
    uint8_t entree[256];
    uint8_t chiffre[256 * RSA_OCTET_CHIFFRE_TAILLE];
    uint8_t sortie[256];
    const uint8_t texte[] = "Adam : ete, cafe, eleve.\xc3\xa9\xc3\xa8\xc3\xa0";
    const uint8_t chiffre_a[] = {0xe6, 0x0a, 0, 0, 0, 0, 0, 0};
    size_t taille_chiffre;
    size_t taille_clair;
    size_t i;

    for (i = 0; i < sizeof entree; ++i) {
        entree[i] = (uint8_t)i;
    }
    VERIFIER(bilan, chiffrer_tableau(entree, sizeof entree, &cle_publique,
                                    chiffre, sizeof chiffre, &taille_chiffre) == RSA_SUCCES);
    VERIFIER(bilan, taille_chiffre == sizeof chiffre);
    VERIFIER(bilan, dechiffrer_tableau(chiffre, taille_chiffre, &cle_privee,
                                      sortie, sizeof sortie, &taille_clair) == RSA_SUCCES);
    VERIFIER(bilan, taille_clair == sizeof entree);
    VERIFIER(bilan, memcmp(entree, sortie, sizeof entree) == 0);
    VERIFIER(bilan, chiffrer_tableau(texte, sizeof texte - 1, &cle_publique,
                                    chiffre, sizeof chiffre, &taille_chiffre) == RSA_SUCCES);
    VERIFIER(bilan, dechiffrer_tableau(chiffre, taille_chiffre, &cle_privee,
                                      sortie, sizeof sortie, &taille_clair) == RSA_SUCCES);
    VERIFIER(bilan, taille_clair == sizeof texte - 1);
    VERIFIER(bilan, memcmp(texte, sortie, taille_clair) == 0);
    entree[0] = 65;
    VERIFIER(bilan, chiffrer_tableau(entree, 1, &cle_publique, chiffre,
                                    sizeof chiffre, &taille_chiffre) == RSA_SUCCES);
    VERIFIER(bilan, memcmp(chiffre, chiffre_a, sizeof chiffre_a) == 0);
    VERIFIER(bilan, chiffrer_tableau(NULL, 0, &cle_publique, NULL,
                                    0, &taille_chiffre) == RSA_SUCCES);
    VERIFIER(bilan, taille_chiffre == 0);
    VERIFIER(bilan, dechiffrer_tableau(NULL, 0, &cle_privee, NULL,
                                      0, &taille_clair) == RSA_SUCCES);
    VERIFIER(bilan, taille_clair == 0);
}

static void tester_erreurs(bilan_t *bilan)
{
    uint8_t entree[16] = {0};
    uint8_t sortie[16];
    uint8_t temoin[16];
    size_t taille = 123;
    uint64_t chiffre = 123;
    uint8_t clair = 123;
    rsaKey_t invalide = {17, 255};
    rsaKey_t identite = {1, 3233};
    rsaKey_t limite = {2, RSA_PHASE1_MODULE_MAX};

    memset(sortie, 0xa5, sizeof sortie);
    memcpy(temoin, sortie, sizeof sortie);
    VERIFIER(bilan, chiffrer_octet(65, NULL, &chiffre) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, chiffre == 123);
    VERIFIER(bilan, chiffrer_octet(65, &cle_publique, NULL) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, dechiffrer_octet(0, &cle_privee, NULL) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, chiffrer_octet(65, &invalide, &chiffre) == RSA_ERREUR_CLE);
    invalide.N = RSA_PHASE1_MODULE_MAX + 1;
    VERIFIER(bilan, chiffrer_octet(65, &invalide, &chiffre) == RSA_ERREUR_CLE);
    invalide.N = 3233;
    invalide.E = 0;
    VERIFIER(bilan, chiffrer_octet(65, &invalide, &chiffre) == RSA_ERREUR_CLE);
    invalide.E = invalide.N;
    VERIFIER(bilan, chiffrer_octet(65, &invalide, &chiffre) == RSA_ERREUR_CLE);
    VERIFIER(bilan, dechiffrer_octet(cle_privee.N, &cle_privee,
                                    &clair) == RSA_ERREUR_CHIFFRE);
    VERIFIER(bilan, clair == 123);
    VERIFIER(bilan, dechiffrer_octet(256, &identite, &clair) == RSA_ERREUR_CHIFFRE);
    VERIFIER(bilan, clair == 123);
    VERIFIER(bilan, dechiffrer_octet(RSA_PHASE1_MODULE_MAX - 1,
                                    &limite, &clair) == RSA_SUCCES);
    VERIFIER(bilan, clair == 1);
    VERIFIER(bilan, chiffrer_tableau(entree, 2, &cle_publique, sortie,
                                    15, &taille) == RSA_ERREUR_TAILLE);
    VERIFIER(bilan, taille == 0);
    VERIFIER(bilan, memcmp(sortie, temoin, sizeof sortie) == 0);
    VERIFIER(bilan, chiffrer_tableau(entree, SIZE_MAX / 8 + 1, &cle_publique,
                                    sortie, SIZE_MAX, &taille) == RSA_ERREUR_TAILLE);
    VERIFIER(bilan, chiffrer_tableau(NULL, 1, &cle_publique, sortie,
                                    sizeof sortie, &taille) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, chiffrer_tableau(entree, 1, &cle_publique, NULL,
                                    8, &taille) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, chiffrer_tableau(entree, 1, &cle_publique, sortie,
                                    sizeof sortie, NULL) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, chiffrer_tableau(entree, 1, &cle_publique, entree,
                                    sizeof entree, &taille) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, dechiffrer_tableau(entree, 7, &cle_privee, sortie,
                                      sizeof sortie, &taille) == RSA_ERREUR_CHIFFRE);
    VERIFIER(bilan, dechiffrer_tableau(entree, 16, &cle_privee, sortie,
                                      1, &taille) == RSA_ERREUR_TAILLE);
    VERIFIER(bilan, dechiffrer_tableau(NULL, 8, &cle_privee, sortie,
                                      sizeof sortie, &taille) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, dechiffrer_tableau(entree, 8, &cle_privee, NULL,
                                      1, &taille) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, dechiffrer_tableau(entree, 8, &cle_privee, sortie,
                                      sizeof sortie, NULL) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, dechiffrer_tableau(entree, 8, &cle_privee, entree,
                                      sizeof entree, &taille) == RSA_ERREUR_PARAMETRE);
    memset(entree + 8, 0xff, 8);
    VERIFIER(bilan, dechiffrer_tableau(entree, sizeof entree, &cle_privee,
                                      sortie, sizeof sortie, &taille) == RSA_ERREUR_CHIFFRE);
    VERIFIER(bilan, taille == 0);
    VERIFIER(bilan, memcmp(sortie, temoin, sizeof sortie) == 0);
}

static int ecrire_fichier_test(const char *chemin, const uint8_t *donnees, size_t taille)
{
    FILE *fichier = fopen(chemin, "wb");
    int resultat;

    if (fichier == NULL) {
        return 0;
    }
    resultat = taille == 0 || fwrite(donnees, 1, taille, fichier) == taille;
    if (fclose(fichier) != 0) {
        resultat = 0;
    }
    return resultat;
}

static int comparer_fichier(const char *chemin, const uint8_t *attendu, size_t taille)
{
    FILE *fichier = fopen(chemin, "rb");
    uint8_t tampon[1024];
    size_t position = 0;
    int resultat = 1;

    if (fichier == NULL) {
        return 0;
    }
    while (position < taille) {
        size_t reste = taille - position;
        size_t lecture = reste < sizeof tampon ? reste : sizeof tampon;
        if (fread(tampon, 1, lecture, fichier) != lecture ||
            memcmp(tampon, attendu + position, lecture) != 0) {
            resultat = 0;
            break;
        }
        position += lecture;
    }
    if (fgetc(fichier) != EOF || ferror(fichier)) {
        resultat = 0;
    }
    if (fclose(fichier) != 0) {
        resultat = 0;
    }
    return resultat;
}

static int chemin_test(char *sortie, size_t capacite,
                       const char *dossier, const char *nom)
{
    int taille = snprintf(sortie, capacite, "%s/%s", dossier, nom);
    return taille >= 0 && (size_t)taille < capacite;
}

static void tester_fichiers(bilan_t *bilan, const char *dossier)
{
    char original[1024];
    char chiffre[1024];
    char retour[1024];
    char absent[1024];
    uint8_t donnees[10001];
    uint8_t chiffre_tableau[256 * RSA_OCTET_CHIFFRE_TAILLE];
    uint8_t invalide[16] = {0};
    const uint8_t temoin[] = "sortie a conserver";
    const uint8_t texte[] = "Bonjour Adam, \xc3\xa9t\xc3\xa9 !\n";
    const size_t tailles[] = {0, 1, 3, 4, 5, 255, 256, 4095, 4096, 4097, 10001};
    size_t taille_chiffre;
    size_t i;
    int chemins_valides;

    chemins_valides = chemin_test(original, sizeof original, dossier, "adam_original.bin") &&
        chemin_test(chiffre, sizeof chiffre, dossier, "adam_chiffre.bin") &&
        chemin_test(retour, sizeof retour, dossier, "adam_retour.bin") &&
        chemin_test(absent, sizeof absent, dossier, "adam_absent/fichier.bin");
    VERIFIER(bilan, chemins_valides);
    if (!chemins_valides) {
        return;
    }
    for (i = 0; i < sizeof donnees; ++i) {
        donnees[i] = (uint8_t)i;
    }
    for (i = 0; i < sizeof tailles / sizeof tailles[0]; ++i) {
        VERIFIER(bilan, ecrire_fichier_test(original, donnees, tailles[i]));
        VERIFIER(bilan, chiffrer_fichier(original, chiffre, &cle_publique) == RSA_SUCCES);
        VERIFIER(bilan, dechiffrer_fichier(chiffre, retour, &cle_privee) == RSA_SUCCES);
        VERIFIER(bilan, comparer_fichier(retour, donnees, tailles[i]));
    }
    VERIFIER(bilan, ecrire_fichier_test(original, donnees, 256));
    VERIFIER(bilan, chiffrer_fichier(original, chiffre, &cle_publique) == RSA_SUCCES);
    VERIFIER(bilan, chiffrer_tableau(donnees, 256, &cle_publique, chiffre_tableau,
                                    sizeof chiffre_tableau, &taille_chiffre) == RSA_SUCCES);
    VERIFIER(bilan, comparer_fichier(chiffre, chiffre_tableau, taille_chiffre));
    VERIFIER(bilan, ecrire_fichier_test(original, texte, sizeof texte - 1));
    VERIFIER(bilan, chiffrer_fichier(original, original, &cle_publique) == RSA_SUCCES);
    VERIFIER(bilan, dechiffrer_fichier(original, original, &cle_privee) == RSA_SUCCES);
    VERIFIER(bilan, comparer_fichier(original, texte, sizeof texte - 1));
    VERIFIER(bilan, ecrire_fichier_test(retour, temoin, sizeof temoin));
    VERIFIER(bilan, ecrire_fichier_test(chiffre, invalide, 7));
    VERIFIER(bilan, dechiffrer_fichier(chiffre, retour, &cle_privee) == RSA_ERREUR_CHIFFRE);
    VERIFIER(bilan, comparer_fichier(retour, temoin, sizeof temoin));
    memset(invalide + 8, 0xff, 8);
    VERIFIER(bilan, ecrire_fichier_test(chiffre, invalide, sizeof invalide));
    VERIFIER(bilan, dechiffrer_fichier(chiffre, retour, &cle_privee) == RSA_ERREUR_CHIFFRE);
    VERIFIER(bilan, comparer_fichier(retour, temoin, sizeof temoin));
    VERIFIER(bilan, chiffrer_fichier(absent, retour, &cle_publique) == RSA_ERREUR_FICHIER);
    VERIFIER(bilan, comparer_fichier(retour, temoin, sizeof temoin));
    VERIFIER(bilan, dechiffrer_fichier(absent, retour, &cle_privee) == RSA_ERREUR_FICHIER);
    VERIFIER(bilan, ecrire_fichier_test(original, donnees, 1));
    VERIFIER(bilan, chiffrer_fichier(original, absent, &cle_publique) == RSA_ERREUR_FICHIER);
    VERIFIER(bilan, chiffrer_fichier(NULL, retour, &cle_publique) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, dechiffrer_fichier(chiffre, "", &cle_privee) == RSA_ERREUR_PARAMETRE);
    VERIFIER(bilan, chiffrer_fichier(original, retour, NULL) == RSA_ERREUR_PARAMETRE);
    {
        rsaKey_t cle_invalide = {0, 3233};
        VERIFIER(bilan, chiffrer_fichier(original, retour, &cle_invalide) == RSA_ERREUR_CLE);
        VERIFIER(bilan, comparer_fichier(retour, temoin, sizeof temoin));
    }
    VERIFIER(bilan, remove(original) == 0);
    VERIFIER(bilan, remove(chiffre) == 0);
    VERIFIER(bilan, remove(retour) == 0);
}

int tester_chiffrement_adam(const char *dossier_temporaire)
{
    bilan_t bilan = {0, 0};

    if (dossier_temporaire == NULL || dossier_temporaire[0] == '\0') {
        fprintf(stderr, "Le dossier temporaire de tests est obligatoire.\n");
        return 1;
    }
    tester_octets(&bilan);
    tester_tableaux(&bilan);
    tester_erreurs(&bilan);
    tester_fichiers(&bilan, dossier_temporaire);
    printf("Chiffrement phase 1 : %u verifications, %u echecs.\n",
           bilan.verifications, bilan.echecs);
    return bilan.echecs != 0;
}
