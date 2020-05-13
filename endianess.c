#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void copy_be16(uint16_t *pdest, uint16_t *psource) {
#if BYTE_ORDER == LITTLE_ENDIAN
  unsigned char *source = (unsigned char *) psource;
  unsigned char *dest = (unsigned char *) pdest;
  dest[0] = source[1];
  dest[1] = source[0];
#else // if BYTE_ORDER == BIG_ENDIAN
#ifdef BINN_ONLY_ALIGNED_ACCESS
  if (psource % 2 == 0){  // address aligned to 16 bit
    *pdest = *psource;
  } else {
    unsigned char *source = (unsigned char *) psource;
    unsigned char *dest = (unsigned char *) pdest;
    dest[0] = source[0];  // indexes are the same
    dest[1] = source[1];
  }
#else
  *pdest = *psource;
#endif
#endif
}

void copy_be32(uint32_t *pdest, uint32_t *psource) {
#if BYTE_ORDER == LITTLE_ENDIAN
  unsigned char *source = (unsigned char *) psource;
  unsigned char *dest = (unsigned char *) pdest;
  dest[0] = source[3];
  dest[1] = source[2];
  dest[2] = source[1];
  dest[3] = source[0];
#else // if BYTE_ORDER == BIG_ENDIAN
#ifdef BINN_ONLY_ALIGNED_ACCESS
  if (psource % 4 == 0){  // address aligned to 32 bit
    *pdest = *psource;
  } else {
    unsigned char *source = (unsigned char *) psource;
    unsigned char *dest = (unsigned char *) pdest;
    dest[0] = source[0];  // indexes are the same
    dest[1] = source[1];
    dest[2] = source[2];
    dest[3] = source[3];
  }
#else
  *pdest = *psource;
#endif
#endif
}

void copy_be64(uint64_t *pdest, uint64_t *psource) {
#if BYTE_ORDER == LITTLE_ENDIAN
  unsigned char *source = (unsigned char *) psource;
  unsigned char *dest = (unsigned char *) pdest;
  int i;
  for (i=0; i < 8; i++) {
    dest[i] = source[7-i];
  }
#else // if BYTE_ORDER == BIG_ENDIAN
#ifdef BINN_ONLY_ALIGNED_ACCESS
  if (psource % 8 == 0){  // address aligned to 64 bit
    *pdest = *psource;
  } else {
    unsigned char *source = (unsigned char *) psource;
    unsigned char *dest = (unsigned char *) pdest;
    int i;
    for (i=0; i < 8; i++) {
      dest[i] = source[i];  // indexes are the same
    }
  }
#else
  *pdest = *psource;
#endif
#endif
}

