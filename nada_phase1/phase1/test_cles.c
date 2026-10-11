/// \file test_cles.c
/// \author Nada ARABA
/// \brief Phase 1.0 : main de test (génération, affichage, sauvegarde / chargement, chiffrement)
///
/// Utilisation :   ./test_cles            (utilise MAX_PRIME = 10000)
///                 ./test_cles 50000      (autre borne pour les nombres premiers)
#include "../rsa_common_header.h"
#include "phase1.h"

static int egales(const rsaKey_t *a, const rsaKey_t *b){
  return a->E == b->E && a->N == b->N;
}

int main(int argc, char *argv[]){
  uint64_t max_prime = MAX_PRIME;
  if (argc > 1)
    max_prime = strtoull(argv[1], NULL, 10);

  srand((unsigned) time(NULL));   // sans srand, rand() donne les MÊMES clefs à chaque lancement
  init_log();
  int erreurs = 0;

  /* ---- 1. génération et affichage d'une paire ---- */
  printf("=== 1. Génération d'une paire de clefs (nombres premiers < %" PRIu64 ") ===\n", max_prime);
  keyPair_t kp;
  gen_keypair(&kp, max_prime);
  print_keypair_hex(&kp);

  /* ---- 2. sauvegarde puis rechargement ---- */
  printf("\n=== 2. Sauvegarde et rechargement ===\n");
  const char *fichier = "cles_test.key";
  if (save_keypair(fichier, &kp) != 0){
    printf("ECHEC : impossible d'écrire %s\n", fichier);
    erreurs++;
  } else {
    keyPair_t relue;
    if (load_keypair(fichier, &relue) != 0 || !egales(&kp.pubKey, &relue.pubKey) || !egales(&kp.privKey, &relue.privKey)){
      printf("ECHEC : les clefs relues sont différentes de celles écrites\n");
      erreurs++;
    } else
      printf("OK : %s relu, clefs identiques\n", fichier);
  }
  // fichier inexistant : doit être refusé proprement, sans planter
  keyPair_t bidon;
  if (load_keypair("fichier_qui_n_existe_pas.key", &bidon) == 0){
    printf("ECHEC : un fichier absent a été accepté\n");
    erreurs++;
  } else
    printf("OK : fichier absent refusé\n");

  /* ---- 3. chiffrement / déchiffrement d'un caractère UTF-8 ---- */
  // 'é' en UTF-8 = 2 octets : 0xC3 0xA9. On chiffre chaque octet séparément (phase 1).
  printf("\n=== 3. Chiffrer / déchiffrer le caractère 'é' (octets 0xC3 0xA9) ===\n");
  const uint8_t e_accent[2] = {0xC3, 0xA9};
  for (int i = 0; i < 2; i++){
    uint64_t c = puissance_mod_n(e_accent[i], kp.pubKey.E,  kp.pubKey.N);   // C = M^E mod N
    uint64_t m = puissance_mod_n(c,           kp.privKey.E, kp.privKey.N);  // M = C^D mod N
    printf("octet 0x%02X -> chiffré %" PRIu64 " -> déchiffré 0x%02" PRIX64 " %s\n",
           e_accent[i], c, m, m == e_accent[i] ? "OK" : "ECHEC");
    if (m != e_accent[i]) erreurs++;
  }

  /* ---- 4. boucle sur de nombreuses paires de clefs ---- */
  printf("\n=== 4. Boucle : 1000 paires de clefs x les 256 valeurs d'un octet ===\n");
  int paires_fausses = 0;
  for (int i = 0; i < 1000; i++){
    keyPair_t k;
    gen_keypair(&k, max_prime);
    for (int octet = 0; octet < 256; octet++){
      uint64_t c = puissance_mod_n(octet, k.pubKey.E,  k.pubKey.N);
      uint64_t m = puissance_mod_n(c,     k.privKey.E, k.privKey.N);
      if (m != (uint64_t) octet){
        paires_fausses++;
        printf("ECHEC pour la paire N=%" PRIu64 " E=%" PRIu64 " D=%" PRIu64 " (octet %d)\n",
               k.pubKey.N, k.pubKey.E, k.privKey.E, octet);
        break;   // une paire fausse suffit, on passe à la suivante
      }
    }
  }
  printf("Résultat : %d paire(s) fausse(s) sur 1000\n", paires_fausses);
  erreurs += paires_fausses;

  printf("\n%s\n", erreurs == 0 ? "TOUS LES TESTS SONT PASSES" : "IL Y A DES ECHECS (voir ci-dessus)");
  close_log();
  return erreurs == 0 ? 0 : 1;
}
