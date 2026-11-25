/// \file test_digest.c
/// \author Nada ARABA
/// \brief Tests de la phase 4 : digest SHA-256 de fichiers (clairs et chiffres), requetes certify / revoke.
///        Laisse digest_binaire.bin / .txt et requetes_test.txt pour une verification independante (Python).
///        Pas de console_utf8.h ici : windows.h et sha256_utils.h definissent chacun un type WORD.
///        Les messages de ce test sont donc sans accents.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../digest.h"
#include "../certification.h"
#include "../sha256_utils.h"
#include "../../phase1_nada/phase1/phase1.h"
#include "../../phase1_adam/chiffrement.h"
#include "../../phase1_loann/base64.h"
#include "../../phase2_nada/gmp_rsa.h"
#include "../../phase2_adam/chiffrement_blocs.h"
#include "../../phase3_interprete/interprete.h"

static unsigned controles = 0, echecs = 0;
#define VERIF(cond) do { ++controles; \
    if (!(cond)) { fprintf(stderr, "ECHEC digest ligne %d : %s\n", __LINE__, #cond); ++echecs; } } while (0)

static int ecrire(const char *chemin, const void *octets, size_t taille) {
    FILE *f = fopen(chemin, "wb");
    if (f == NULL) return 0;
    int ok = fwrite(octets, 1, taille, f) == taille;
    return fclose(f) == 0 && ok;
}

static int hex_de(const void *octets, size_t taille, const char *attendu) {
    uint8_t d[DIGEST_TAILLE];
    char hex[DIGEST_HEX_TAILLE];
    digest_octets(octets, taille, d);
    digest_vers_hex(d, hex);
    return strcmp(hex, attendu) == 0;
}

/* Valeurs de reference publiees pour SHA-256 (FIPS 180-2). */
static void test_vecteurs(void) {
    static char million[1000000];
    memset(million, 'a', sizeof million);
    VERIF(hex_de("", 0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    VERIF(hex_de("abc", 3, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    VERIF(hex_de("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
                 "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
    VERIF(hex_de(million, sizeof million, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
    /* meme resultat que sha256ofString fourni par le sujet, pour du texte (UTF-8 compris) */
    const char *textes[] = { "abc", "L'éléphant lançait des avions en papier 1 2 3 123 !", "x" };
    for (size_t i = 0; i < sizeof textes / sizeof textes[0]; i++) {
        char fourni[SHA256_BLOCK_SIZE * 2 + 1], nouveau[DIGEST_HEX_TAILLE];
        uint8_t d[DIGEST_TAILLE];
        sha256ofString((BYTE *)textes[i], fourni);
        digest_octets((const uint8_t *)textes[i], strlen(textes[i]), d);
        digest_vers_hex(d, nouveau);
        VERIF(strcmp(fourni, nouveau) == 0);
    }
}

static void test_fichiers(void) {
    static uint8_t donnees[20000];
    char hex[DIGEST_HEX_TAILLE], hex2[DIGEST_HEX_TAILLE];
    uint8_t d[DIGEST_TAILLE], d2[DIGEST_TAILLE];
    for (size_t i = 0; i < sizeof donnees; i++) donnees[i] = (uint8_t)(i * 7u + (i >> 9));   /* octets nuls compris */
    /* tailles autour des morceaux de lecture (4096) et des blocs SHA-256 (64) */
    static const size_t tailles[] = { 0, 1, 55, 56, 63, 64, 65, 4095, 4096, 4097, 8192, 20000 };
    for (size_t i = 0; i < sizeof tailles / sizeof tailles[0]; i++) {
        VERIF(ecrire("f.bin", donnees, tailles[i]));
        digest_octets(donnees, tailles[i], d2);
        VERIF(digest_fichier("f.bin", d) == RSA_SUCCES && memcmp(d, d2, DIGEST_TAILLE) == 0);
    }
    VERIF(ecrire("abc.txt", "abc", 3));
    VERIF(digest_fichier_hex("abc.txt", hex) == RSA_SUCCES &&
          strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0);

    /* un octet modifie change tout le digest */
    VERIF(ecrire("f.bin", donnees, 5000) && digest_fichier_hex("f.bin", hex) == RSA_SUCCES);
    donnees[2500] ^= 1;
    VERIF(ecrire("f.bin", donnees, 5000) && digest_fichier_hex("f.bin", hex2) == RSA_SUCCES);
    VERIF(strcmp(hex, hex2) != 0);

    /* erreurs : sortie intacte */
    memset(d, 0x5a, sizeof d);
    VERIF(digest_fichier("fichier_absent.bin", d) == RSA_ERREUR_FICHIER && d[0] == 0x5a && d[31] == 0x5a);
    VERIF(digest_fichier(NULL, d) == RSA_ERREUR_PARAMETRE && digest_fichier("", d) == RSA_ERREUR_PARAMETRE);
    VERIF(digest_fichier("abc.txt", NULL) == RSA_ERREUR_PARAMETRE && digest_fichier_hex("abc.txt", NULL) == RSA_ERREUR_PARAMETRE);

    /* fichier chiffre (sujet : « digest d'un fichier chiffre ou clair ») */
    rsaKey_t pub, priv;
    VERIF(gen_keys_gmp(&pub, &priv, 28) == RSA_SUCCES);
    VERIF(chiffrer_fichier_blocs_gmp("abc.txt", "abc.chiffre", &pub) == RSA_SUCCES);
    VERIF(digest_fichier_hex("abc.chiffre", hex2) == RSA_SUCCES && strlen(hex2) == 64);
    VERIF(strcmp(hex2, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") != 0);
    VERIF(dechiffrer_fichier_blocs_gmp("abc.chiffre", "abc.dechiffre", &priv) == RSA_SUCCES);
    VERIF(digest_fichier_hex("abc.dechiffre", hex2) == RSA_SUCCES &&
          strcmp(hex2, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0);

    /* pour la verification independante par le lanceur Python */
    VERIF(ecrire("digest_binaire.bin", donnees, sizeof donnees));
    VERIF(digest_fichier_hex("digest_binaire.bin", hex) == RSA_SUCCES);
    VERIF(ecrire("digest_binaire.txt", hex, 64));
}

static void test_requetes(void) {
    entree_clef_t e[2];
    memset(e, 0, sizeof e);
    e[0] = (entree_clef_t){ "perso", CLEF_CRYPT, { 0x10001, 0x100040003ULL }, { 0x12d687, 0x100040003ULL }, true };
    e[1] = (entree_clef_t){ "signe", CLEF_SIGN, { 3, 0x1a2b3c4d5ULL }, { 7, 0x1a2b3c4d5ULL }, true };
    table_clefs_t t = { e, 2, 2, NULL };
    const char *code = NULL;
    VERIF(strcmp(certif_code(CERTIF_DECLARATION, CLEF_CRYPT), "NCK") == 0);
    VERIF(strcmp(certif_code(CERTIF_DECLARATION, CLEF_SIGN), "NSK") == 0);
    VERIF(strcmp(certif_code(CERTIF_REVOCATION, CLEF_CRYPT), "RCK") == 0);
    VERIF(strcmp(certif_code(CERTIF_REVOCATION, CLEF_SIGN), "RSK") == 0);

    remove("requetes_test.txt");
    VERIF(certif_requete(&t, "perso", CERTIF_DECLARATION, "requetes_test.txt", 1700000000, &code) == CMD_SUCCES);
    VERIF(code != NULL && strcmp(code, "NCK") == 0);
    VERIF(certif_requete(&t, "signe", CERTIF_DECLARATION, "requetes_test.txt", 1700000001, &code) == CMD_SUCCES);
    VERIF(certif_requete(&t, "perso", CERTIF_REVOCATION, "requetes_test.txt", 1700000002, &code) == CMD_SUCCES);
    VERIF(certif_requete(&t, "signe", CERTIF_REVOCATION, "requetes_test.txt", 1700000003, NULL) == CMD_SUCCES);
    VERIF(certif_requete(&t, "absente", CERTIF_DECLARATION, "requetes_test.txt", 1, &code) == CMD_ERREUR_ID_INCONNU);
    VERIF(certif_requete(&t, "jean/boulot", CERTIF_DECLARATION, "requetes_test.txt", 1, &code) == CMD_ERREUR_ID_INCONNU);
    VERIF(certif_requete(&t, "perso", CERTIF_DECLARATION, "dossier_absent/r.txt", 1, &code) == RSA_ERREUR_FICHIER);
    VERIF(certif_requete(NULL, "perso", CERTIF_DECLARATION, "r.txt", 1, &code) == RSA_ERREUR_PARAMETRE);
    VERIF(certif_requete(&t, "perso", CERTIF_DECLARATION, "", 1, &code) == RSA_ERREUR_PARAMETRE);

    /* contenu exact : 4 lignes, digest recalcule avec sha256ofString du sujet */
    FILE *f = fopen("requetes_test.txt", "r");
    char ligne[CERTIF_LIGNE_MAX + 2];
    const char *debuts[] = { "NCK 1700000000 perso crypt 10001 100040003", "NSK 1700000001 signe sign 3 1a2b3c4d5",
                             "RCK 1700000002 perso crypt 10001 100040003", "RSK 1700000003 signe sign 3 1a2b3c4d5" };
    int n = 0;
    VERIF(f != NULL);
    while (f != NULL && fgets(ligne, sizeof ligne, f) != NULL) {
        char attendu[CERTIF_LIGNE_MAX], hex[SHA256_BLOCK_SIZE * 2 + 1];
        VERIF(n < 4);
        if (n >= 4) break;
        sha256ofString((BYTE *)debuts[n], hex);
        snprintf(attendu, sizeof attendu, "%s %s\n", debuts[n], hex);
        VERIF(strcmp(ligne, attendu) == 0);
        VERIF(certif_verifier_ligne(ligne));
        /* une requete modifiee (ici la clef) n'est plus valide */
        char modifiee[CERTIF_LIGNE_MAX + 2];
        snprintf(modifiee, sizeof modifiee, "%s", ligne);
        char *espace = strrchr(modifiee, ' ');
        if (espace != NULL && espace > modifiee) espace[-1] = espace[-1] == '3' ? '4' : '3';
        VERIF(!certif_verifier_ligne(modifiee));
        n++;
    }
    if (f != NULL) fclose(f);
    VERIF(n == 4);
    VERIF(!certif_verifier_ligne(NULL) && !certif_verifier_ligne("") && !certif_verifier_ligne("NCK 1 a crypt 3 21"));
    VERIF(!certif_verifier_ligne("XYZ 1700000000 perso crypt 10001 100040003 "
                                 "0000000000000000000000000000000000000000000000000000000000000000"));
}

/* certify / revoke depuis l'interprete */
static void test_interprete(void) {
    interprete_t it;
    char l1[] = "newkeys perso crypt", l2[] = "newkeys signe sign", l3[] = "certify perso", l4[] = "revoke signe";
    char l5[] = "certify", l6[] = "certify absente", l7[] = "revoke perso trop";
    interprete_initialiser(&it);
    it.requetes = "requetes_interprete.txt";
    remove(it.requetes);
    FILE *sortie = fopen("sortie_certif.txt", "w+");
    VERIF(sortie != NULL);
    if (sortie == NULL) return;
    VERIF(interprete_ligne(&it, l1, stdin, sortie) == CMD_SUCCES && interprete_ligne(&it, l2, stdin, sortie) == CMD_SUCCES);
    VERIF(interprete_ligne(&it, l3, stdin, sortie) == CMD_SUCCES);
    VERIF(interprete_ligne(&it, l4, stdin, sortie) == CMD_SUCCES);
    VERIF(interprete_ligne(&it, l5, stdin, sortie) == CMD_ERREUR_SYNTAXE);
    VERIF(interprete_ligne(&it, l6, stdin, sortie) == CMD_ERREUR_ID_INCONNU);
    VERIF(interprete_ligne(&it, l7, stdin, sortie) == CMD_ERREUR_SYNTAXE);
    rewind(sortie);
    char texte[512];
    size_t lus = fread(texte, 1, sizeof texte - 1, sortie);
    texte[lus] = '\0';
    fclose(sortie);
    VERIF(strcmp(texte, "Requete NCK pour perso ajoutee a requetes_interprete.txt\n"
                        "Requete RSK pour signe ajoutee a requetes_interprete.txt\n") == 0);
    FILE *f = fopen(it.requetes, "r");
    char ligne[CERTIF_LIGNE_MAX + 2];
    int n = 0;
    while (f != NULL && fgets(ligne, sizeof ligne, f) != NULL) { VERIF(certif_verifier_ligne(ligne)); n++; }
    if (f != NULL) fclose(f);
    VERIF(n == 2);
    interprete_liberer(&it);
}

int main(void) {
    init_log();
    test_vecteurs();
    test_fichiers();
    test_requetes();
    test_interprete();
    base64_cleanup();
    close_log();
    printf("Phase 4 (digest, certify, revoke) : %u verifications, %u echecs.\n", controles, echecs);
    return echecs == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
