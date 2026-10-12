#include "../nada_phase1/rsa_common_header.h"
#include "../nada_phase1/phase1/phase1.h"
#include "base64.h"
#include <stdio.h>
#include <stdlib.h>

int file_to_base64(const char *in, const char *out) {
    FILE *f = fopen(in, "rb");
    if (f == NULL) {
        perror("Erreur : le fichier entrant n'existe pas");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *read_data = malloc(len);
    if (read_data == NULL) {
        perror("Erreur d'allocation mémoire");
        fclose(f);
        return -1;
    }

    fread(read_data, 1, len, f);
    fclose(f);

    size_t b64_len = 0;
    char *b64_data = base64_encode(read_data, len, &b64_len);
    free(read_data);

    if (b64_data == NULL) {
        return -1;
    }

    FILE *fout = fopen(out, "w");
    if (fout == NULL) {
        perror("Erreur lors de la création du fichier sortant");
        free(b64_data);
        return -1;
    }

    fwrite(b64_data, 1, b64_len, fout);
    fclose(fout);
    free(b64_data);

    return 0;
}

int base64_to_file(const char *in, const char *out) {
    FILE *f = fopen(in, "r");
    if (f == NULL) {
        perror("Erreur : le fichier Base64 entrant n'existe pas");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *read_data = malloc(len + 1);
    if (read_data == NULL) {
        perror("Erreur d'allocation mémoire");
        fclose(f);
        return -1;
    }

    size_t bytes_read = fread(read_data, 1, len, f);
    read_data[bytes_read] = '\0';
    fclose(f);

    size_t bin_len = 0;
    
    unsigned char *bin_data = base64_decode(read_data, bytes_read, &bin_len);
    free(read_data);

    if (bin_data == NULL) {
        return -1;
    }

    FILE *fout = fopen(out, "wb");
    if (fout == NULL) {
        perror("Erreur lors de la création du fichier binaire sortant");
        free(bin_data);
        return -1;
    }

    fwrite(bin_data, 1, bin_len, fout);
    fclose(fout);
    free(bin_data);

    return 0;
}