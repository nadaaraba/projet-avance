#include <stdio.h>
#include <stdlib.h>
#include "rsa_common_header.h"
#include "phase1.h"

FILE *logfp = NULL;

int main() {
    logfp = stdout;
    uint64_t max_prime = 10000;
    keyPair_t kp;

    printf("=== LANCEMENT DES TESTS PHASE 1 (MALEK) ===\n\npr\n");

    // 1. Génération des clés avec Rabin
    printf("1. Generation des cles...\n");
    genKeysRabin(&kp.pubKey, &kp.privKey, max_prime);
    printKeyHex(&kp.pubKey, "Publique");
    printKeyHex(&kp.privKey, "Privee");

    // 2. Test sauvegarde et chargement
    printf("\n2. Test sauvegarde et chargement des cles...\n");
    if (saveKeyPair("keys_test.txt", &kp)) {
        printf("-> Cles sauvegardees avec succes dans keys_test.txt\n");
    }
    keyPair_t kp_loaded;
    if (loadKeyPair("keys_test.txt", &kp_loaded)) {
        printf("-> Cles chargees avec succes !\n");
        printKeyHex(&kp_loaded.pubKey, "Chargee Publique");
    }

    // 3. Test de chiffrement / déchiffrement de fichiers
    printf("\n3. Test chiffrement/dechiffrement de fichiers...\n");
    FILE *f = fopen("msg_clair.txt", "w");
    fprintf(f, "Bonjour, ceci est un test secret pour la phase 1 !");
    fclose(f);

    if (encryptFile("msg_clair.txt", "msg_chiffre.txt", &kp.pubKey)) {
        printf("-> Fichier chiffre avec succes dans msg_chiffre.txt\n");
    }
    if (decryptFile("msg_chiffre.txt", "msg_dechiffre.txt", &kp.privKey)) {
        printf("-> Fichier dechiffre avec succes dans msg_dechiffre.txt\n");
    }

    // 4. Test Base64
    printf("\n4. Test conversion Base64...\n");
    if (file_to_base64("msg_chiffre.txt", "msg_chiffre.b64")) {
        printf("-> Fichier converti en Base64 (msg_chiffre.b64)\n");
    }
    if (base64_to_file("msg_chiffre.b64", "msg_recup.bin")) {
        printf("-> Fichier Base64 reconverti en binaire (msg_recup.bin)\n");
    }

    printf("\n=== TOUS LES TESTS SONT TERMINES AVEC SUCCES ===\n");
    return 0;
}