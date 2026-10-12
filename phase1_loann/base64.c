#include "phase1"
#include "rsa_common_header.h"
#include "base64.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <file/file.h>
#include <stddef.h>

int file_to_base64(const char *in, const char *out) {
    FILE* f = fopen(in, "rb");
    if (f == NULL) {
        perror("le fichier n'existe pas\n");
        exit(1);
    }

    int len;
    fseek(f, 0, SEEK_END);
    len =ftell(f);
    fseek(f, 0, SEEK_SET);

    char* read_data = malloc(len);

    if (read_data == NULL) {
        perror("le fichier n'existe pas\n");
        exit(1);
    }

    fread(read_data, 1, len, f);
    fclose(f);

    size_t b64_len = 0;

    char *b64_data = base64_encode(read_data, len, &b64_len);
    free(read_data);

    FILE *fout = fopen(out, "w");
    fwrite(b64_data, 1 ,b64_data, fout);
    fclose(fout);
    free(b64_data);

    return 0;
}

int base64_to_file(const char *in, const char *out) {
    FILE* f = fopen(in, "rb");
    if (f == NULL) {
        perror("le fichier n'existe pas\n");
        exit(1);
    }

    int len;
    fseek(f, 0, SEEK_END);
    len =ftell(f);
    fseek(f, 0, SEEK_SET);

    char* read_data = malloc(len);

    if (read_data == NULL) {
        perror("le fichier n'existe pas\n");
        exit(1);
    }

    fread(read_data, 1, len, f);
    fclose(f);

    size_t b64_len = 0;

    char *b64_data = base64_decode(read_data, len, &b64_len);
    free(read_data);

    FILE *fout = fopen(out, "w");
    fwrite(b64_data, 1 ,b64_data, fout);
    fclose(fout);
    free(b64_data);

    return 0;
    return
}