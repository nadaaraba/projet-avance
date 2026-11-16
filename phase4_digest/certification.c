/// \file certification.c
/// \author Nada ARABA
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "certification.h"
#include "digest.h"
#include "../phase1_adam/chiffrement.h"   /* codes RSA_* */

const char *certif_code(certif_action_t action, type_clef_t type) {
    if (action == CERTIF_REVOCATION) return type == CLEF_SIGN ? "RSK" : "RCK";
    return type == CLEF_SIGN ? "NSK" : "NCK";
}

/* Digest hexadecimal des n premiers caracteres de la ligne. */
static void digest_texte(const char *texte, size_t n, char hex[DIGEST_HEX_TAILLE]) {
    uint8_t digest[DIGEST_TAILLE];
    digest_octets((const uint8_t *)texte, n, digest);
    digest_vers_hex(digest, hex);
}

int certif_requete(const table_clefs_t *table, const char *keyid, certif_action_t action,
                   const char *fichier, time_t date, const char **code) {
    char ligne[CERTIF_LIGNE_MAX], hex[DIGEST_HEX_TAILLE];
    if (table == NULL || keyid == NULL || fichier == NULL || fichier[0] == '\0' ||
        (action != CERTIF_DECLARATION && action != CERTIF_REVOCATION)) return RSA_ERREUR_PARAMETRE;
    const entree_clef_t *e = table_chercher(table, keyid);   /* clefs de l'utilisateur seulement */
    if (e == NULL) return CMD_ERREUR_ID_INCONNU;
    const char *c = certif_code(action, e->type);
    int n = snprintf(ligne, sizeof ligne, "%s %lld %s %s %" PRIx64 " %" PRIx64, c, (long long)date, e->id,
                     e->type == CLEF_SIGN ? "sign" : "crypt", e->pub.E, e->pub.N);
    if (n < 0 || (size_t)n >= sizeof ligne) return RSA_ERREUR_PARAMETRE;
    digest_texte(ligne, (size_t)n, hex);

    FILE *f = fopen(fichier, "a");   /* file d'attente : les requetes s'ajoutent a la fin */
    if (f == NULL) return RSA_ERREUR_FICHIER;
    int ok = fprintf(f, "%s %s\n", ligne, hex) >= 0;
    if (fclose(f) != 0) ok = 0;
    if (!ok) return RSA_ERREUR_FICHIER;
    if (code != NULL) *code = c;
    return CMD_SUCCES;
}

int certif_verifier_ligne(const char *ligne) {
    char code[4], type[6], id[NAME_MAX_SIZE + 1], hex[DIGEST_HEX_TAILLE], attendu[DIGEST_HEX_TAILLE];
    long long date;
    uint64_t e, n;
    int fin = 0;
    if (ligne == NULL) return 0;
    size_t taille = strcspn(ligne, "\n");
    if (taille < DIGEST_HEX_TAILLE || taille >= CERTIF_LIGNE_MAX) return 0;
    if (sscanf(ligne, "%3s %lld %32s %5s %" SCNx64 " %" SCNx64 " %64s%n",
               code, &date, id, type, &e, &n, hex, &fin) != 7 || (size_t)fin != taille) return 0;
    if (strcmp(type, "crypt") != 0 && strcmp(type, "sign") != 0) return 0;
    type_clef_t t = strcmp(type, "sign") == 0 ? CLEF_SIGN : CLEF_CRYPT;
    if (strcmp(code, certif_code(CERTIF_DECLARATION, t)) != 0 &&
        strcmp(code, certif_code(CERTIF_REVOCATION, t)) != 0) return 0;
    digest_texte(ligne, taille - DIGEST_HEX_TAILLE, attendu);   /* sans " <digest>" */
    return strcmp(hex, attendu) == 0;
}
