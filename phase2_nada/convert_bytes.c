/// \file convert_bytes.c
#include "convert_bytes.h"

uint32_t convert_4byte2int(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

void convertInt2uchar(uint32_t nb, uint8_t *tab4bytes) {
    for (int i = 0; i < 4; i++) {
        tab4bytes[i] = (uint8_t)(nb & 0xFFu);
        nb >>= 8;
    }
}
