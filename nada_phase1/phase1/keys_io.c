/// \file keys_io.c
/// \author Nada ARABA
/// \brief Phase 1.0 : génération, affichage hexadécimal, sauvegarde et chargement des clefs RSA
#include <sys/stat.h>
#include "../rsa_common_header.h"
#include "phase1.h"

/// Format du fichier de clefs (texte, lisible à l'oeil, valeurs en hexadécimal) :
///     DIYPG-KEYPAIR 1
///     PUB  <E> <N>
///     PRIV <E> <N>
/// Le "1" est un numéro de version : si on change le format plus tard (phase 3 : sauvegarde
/// de tout l'annuaire), on pourra encore lire les anciens fichiers.

void print_key_hex(const char *label, const rsaKey_t *key){
  /// \brief affiche une clef en hexadécimal, ex :  clef publique : E = 0x3  N = 0x1a2b3c
  printf("%s : E = 0x%" PRIx64 "  N = 0x%" PRIx64 "\n", label, key->E, key->N);
}

void print_keypair_hex(const keyPair_t *kp){
  /// \brief affiche la paire de clefs mais affiche aussi la clef privée
  /// \brief à ne servir que pour les tests / la commande "show" de l'interpréteur.
  print_key_hex("clef publique", &kp->pubKey);
  print_key_hex("clef privee  ", &kp->privKey);
}

void gen_keypair(keyPair_t *kp, uint64_t max_prime){
  /// \brief génère une paire de clefs (petite enveloppe autour de genKeysRabin du code fourni)
  genKeysRabin(&kp->pubKey, &kp->privKey, max_prime);
}

int save_keypair(const char *path, const keyPair_t *kp){
  /// \brief écrit la paire de clefs dans un fichier texte
  /// \returns 0 si tout s'est bien passé, -1 sinon
  FILE *f = fopen(path, "w");
  if (f == NULL)
    return -1;
  fprintf(f, "DIYPG-KEYPAIR 1\n");
  fprintf(f, "PUB %" PRIx64 " %" PRIx64 "\n",  kp->pubKey.E,  kp->pubKey.N);
  fprintf(f, "PRIV %" PRIx64 " %" PRIx64 "\n", kp->privKey.E, kp->privKey.N);
  if (fclose(f) != 0)     // fclose peut échouer (disque plein...) : on le vérifie
    return -1;
  // Le fichier contient la clef privée : lecture / écriture réservées à l'utilisateur (rw-------)
  chmod(path, S_IRUSR | S_IWUSR);
  return 0;
}

int load_keypair(const char *path, keyPair_t *kp){
  /// \brief relit une paire de clefs sauvegardée par save_keypair
  /// \returns 0 si ok, -1 si fichier absent ou mal formé. kp n'est modifié QUE en cas de succès.
  FILE *f = fopen(path, "r");
  if (f == NULL)
    return -1;

  char tag[32], nom1[8], nom2[8];
  int version;
  keyPair_t tmp;
  int ok =
    fscanf(f, "%31s %d", tag, &version) == 2 &&
    strcmp(tag, "DIYPG-KEYPAIR") == 0 && version == 1 &&
    fscanf(f, "%7s %" SCNx64 " %" SCNx64, nom1, &tmp.pubKey.E,  &tmp.pubKey.N)  == 3 &&
    strcmp(nom1, "PUB") == 0 &&
    fscanf(f, "%7s %" SCNx64 " %" SCNx64, nom2, &tmp.privKey.E, &tmp.privKey.N) == 3 &&
    strcmp(nom2, "PRIV") == 0;
  fclose(f);

  // Un fichier mal formé ou rempli de zéros ne doit pas donner de "clefs" silencieusement
  if (!ok || tmp.pubKey.N == 0 || tmp.pubKey.E == 0 || tmp.privKey.E == 0 || tmp.pubKey.N != tmp.privKey.N)
    return -1;

  *kp = tmp;
  return 0;
}
