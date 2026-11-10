/// \file convert_bytes.h
/// \brief Conversions 4 octets <-> entier 32 bits, ordre LITTLE ENDIAN (cahier des charges phase 2).
/// \note Version de remplacement : le sujet indique que ces deux fonctions sont données sur Moodle.
///       Si vous utilisez le fichier de Moodle, supprimez convert_bytes.c et gardez ses prototypes
///       (uint convert_4byte2int(uchar *b) ; void convertInt2uchar(uint nb, uchar *tab4bytes)).
#ifndef CONVERT_BYTES_H
#define CONVERT_BYTES_H

#include <stdint.h>

/* octet 0 = poids faible : {0x78,0x56,0x34,0x12} -> 0x12345678 */
uint32_t convert_4byte2int(const uint8_t *b);
void convertInt2uchar(uint32_t nb, uint8_t *tab4bytes);

#endif
