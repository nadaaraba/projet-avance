#ifndef __BASE64__

#define __BASE64__

#include "../nada_phase1/rsa_common_header.h"
#include "../nada_phase1/phase1/phase1.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length);

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length);

int file_to_base64(const char *in, const char *out);

int base64_to_file(const char *in, const char *out);

int test_base64(void);

#endif