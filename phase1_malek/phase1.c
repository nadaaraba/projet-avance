#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rsa_common_header.h"
#include "phase1.h"

// Déclarations externes des fonctions de base64 si besoin (ou inclus via header si dispo)
char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length);
unsigned char *base64_decode(const char *data, size_t input_length, size_t *output_length);

// 1. Affichage des clés en hexadécimal
void printKeyHex(rsaKey_t *key, const char *name) {
    printf("--- Clé %s ---\n", name);
    printf("Exposant (E) : 0x%llX (%llu)\n", key->E, key->E);
    printf("Modulo (N)     : 0x%llX (%llu)\n", key->N, key->N);
}

// 2. Sauvegarder une paire de clés dans un fichier
int saveKeyPair(const char *filename, keyPair_t *kp) {
    FILE *f = fopen(filename, "w");
    if (!f) return 0;
    fprintf(f, "%llu %llu\n", kp->pubKey.E, kp->pubKey.N);
    fprintf(f, "%llu %llu\n", kp->privKey.E, kp->privKey.N);
    fclose(f);
    return 1;
}

// 3. Charger une paire de clés depuis un fichier
int loadKeyPair(const char *filename, keyPair_t *kp) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    if (fscanf(f, "%llu %llu", &kp->pubKey.E, &kp->pubKey.N) != 2) {
        fclose(f);
        return 0;
    }
    if (fscanf(f, "%llu %llu", &kp->privKey.E, &kp->privKey.N) != 2) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return 1;
}

// 4. Chiffrer un tableau d'octets
uint8_t* encryptByteArray(const uint8_t *input, size_t length, rsaKey_t *key, size_t *out_len) {
    *out_len = length;
    uint8_t *output = malloc(length);
    if (!output) return NULL;

    for (size_t i = 0; i < length; i++) {
        output[i] = (uint8_t)puissance_mod_n(input[i], key->E, key->N);
    }
    return output;
}

// 5. Déchiffrer un tableau d'octets
uint8_t* decryptByteArray(const uint8_t *input, size_t length, rsaKey_t *key, size_t *out_len) {
    *out_len = length;
    uint8_t *output = malloc(length);
    if (!output) return NULL;

    for (size_t i = 0; i < length; i++) {
        output[i] = (uint8_t)puissance_mod_n(input[i], key->E, key->N);
    }
    return output;
}

// 6. Chiffrer un message lu dans un fichier vers un autre fichier
int encryptFile(const char *fileIn, const char *fileOut, rsaKey_t *key) {
    FILE *fin = fopen(fileIn, "rb");
    FILE *fout = fopen(fileOut, "wb");
    if (!fin || !fout) {
        if (fin) fclose(fin);
        if (fout) fclose(fout);
        return 0;
    }

    int c;
    while ((c = fgetc(fin)) != EOF) {
        uint64_t encrypted = puissance_mod_n((uint8_t)c, key->E, key->N);
        fputc((uint8_t)encrypted, fout);
    }

    fclose(fin);
    fclose(fout);
    return 1;
}

// 7. Déchiffrer un message lu dans un fichier vers un autre fichier
int decryptFile(const char *fileIn, const char *fileOut, rsaKey_t *key) {
    FILE *fin = fopen(fileIn, "rb");
    FILE *fout = fopen(fileOut, "wb");
    if (!fin || !fout) {
        if (fin) fclose(fin);
        if (fout) fclose(fout);
        return 0;
    }

    int c;
    while ((c = fgetc(fin)) != EOF) {
        uint64_t decrypted = puissance_mod_n((uint8_t)c, key->E, key->N);
        fputc((uint8_t)decrypted, fout);
    }

    fclose(fin);
    fclose(fout);
    return 1;
}

// 8. Convertir un fichier binaire en fichier base64
int file_to_base64(const char *fileIn, const char *fileOut) {
    FILE *fin = fopen(fileIn, "rb");
    if (!fin) return 0;

    fseek(fin, 0, SEEK_END);
    size_t length = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    unsigned char *buffer = malloc(length);
    if (!buffer) { fclose(fin); return 0; }
    fread(buffer, 1, length, fin);
    fclose(fin);

    size_t out_len;
    char *encoded = base64_encode(buffer, length, &out_len);
    free(buffer);
    if (!encoded) return 0;

    FILE *fout = fopen(fileOut, "w");
    if (!fout) { free(encoded); return 0; }
    fwrite(encoded, 1, out_len, fout);
    fclose(fout);
    free(encoded);
    return 1;
}

// 9. Convertir un fichier base64 en fichier binaire
int base64_to_file(const char *fileIn, const char *fileOut) {
    FILE *fin = fopen(fileIn, "r");
    if (!fin) return 0;

    fseek(fin, 0, SEEK_END);
    size_t length = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    char *buffer = malloc(length + 1);
    if (!buffer) { fclose(fin); return 0; }
    size_t read_len = fread(buffer, 1, length, fin);
    buffer[read_len] = '\0';
    fclose(fin);

    size_t out_len;
    unsigned char *decoded = base64_decode(buffer, read_len, &out_len);
    free(buffer);
    if (!decoded) return 0;

    FILE *fout = fopen(fileOut, "wb");
    if (!fout) { free(decoded); return 0; }
    fwrite(decoded, 1, out_len, fout);
    fclose(fout);
    free(decoded);
    return 1;
}