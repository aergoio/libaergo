//#include <stdio.h>
//#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include "mbedtls/bignum.h"

static const char hexdigits[] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' 
};

// convert a value from big endian variable integer to string with
// decimal point. uses 18 decimal digits
int bignum_to_string(uint8_t *buf, int len, uint8_t *out, int outlen) {
    mbedtls_mpi value;
    char tmp[36];
    size_t slen;
    int i;

    mbedtls_mpi_init(&value);
    if (mbedtls_mpi_read_binary(&value, buf, len) != 0) goto loc_failed;
    if (mbedtls_mpi_write_string(&value, 10, out, outlen, &slen) != 0) goto loc_failed;
    mbedtls_mpi_free(&value);

    // align the string to the right
    snprintf(tmp, sizeof(tmp), "%32s", out);

    // replace the spaces with zeros
    for (i = 0; tmp[i] == ' '; i++) {
        tmp[i] = '0';
        slen++;
    }

    // add the decimal point
    snprintf(out, outlen, "%.*s.%s", 32-18, tmp, tmp+32-18);

    // return the size
    return slen + 1;

loc_failed:
    mbedtls_mpi_free(&value);
    return -1;
}

// convert a value from string format to big endian variable integer.
// the string can optionally have a decimal point. uses 18 decimal digits
int string_to_bignum(uint8_t *str, int len, uint8_t *out, int outlen) {
    mbedtls_mpi value;
    uint8_t integer[36], decimal[36], *p;
    int i;

    p = strchr(str, '.');
    if (p) {
        // copy the integer part
        snprintf(integer, sizeof(integer), "%.*s", (int)(p-str), str);
        // copy the decimal part
        p++;
        for (i = 0; i < 18 && *p; i++) {
            decimal[i] = *p++;
        }
    } else {
        strcpy(integer, str);
        i = 0;
    }

    // fill the decimal with trailing zeros
    for (; i < 18; i++) {
        decimal[i] = '0';
    }
    // null terminator
    decimal[18] = 0;

    // contatenate both parts
    strcat(integer, decimal);

    // convert it to big endian variable size integer
    mbedtls_mpi_init(&value);
    if (mbedtls_mpi_read_string(&value, 10, integer) != 0) goto loc_failed;
    if (mbedtls_mpi_write_binary(&value, out, outlen) != 0) goto loc_failed;
    len = mbedtls_mpi_size(&value);
    mbedtls_mpi_free(&value);

    return len;

loc_failed:
    mbedtls_mpi_free(&value);
    return -1;
}

bool arguments_to_json(char *buf, int bufsize, char *types, va_list ap){
  char c, *dest;
  int i, dsize;

  buf[0] = '[';
  dest = &buf[1];
  dsize = bufsize - 2;

  for(i=0; (c = types[i])!=0; i++){
    if( c=='s' ){
      char *limit = buf + bufsize - 4;
      char *z = va_arg(ap, char*);
      if (i > 0) *dest++ = ',';
      *dest++ = '"';
      while (*z) {
        *dest++ = *z++;
        if (dest > limit) goto loc_invalid;
      }
      *dest++ = '"';
    }else if( c=='i' ){
      if (dsize < 12) goto loc_invalid;
      if (i > 0) *dest++ = ',';
      snprintf(dest, dsize, "%d", va_arg(ap, int));
      dest += strlen(dest);
    }else if( c=='l' ){
      if (dsize < 12) goto loc_invalid;
      if (i > 0) *dest++ = ',';
      snprintf(dest, dsize, "%lld", va_arg(ap, int64_t));
      dest += strlen(dest);
    //}else if( c=='f' ){  -- floats are promoted to double
    }else if( c=='d' ){
      if (dsize < 26) goto loc_invalid;
      if (i > 0) *dest++ = ',';
      snprintf(dest, dsize, "%f", va_arg(ap, double));
      dest += strlen(dest);
    }else if( c=='b' ){
      char *ptr = va_arg(ap, char*);
      int size = va_arg(ap, int);
      if (dsize < (size * 2) + 3) goto loc_invalid;
      if (i > 0) *dest++ = ',';
      *dest++ = '"';
      while (size) {
        c = *ptr++;
        *(dest++) = hexdigits[(c>>4)&0xf];
        *(dest++) = hexdigits[c&0xf];
        size--;
      }
      *dest++ = '"';
    }else{
      goto loc_invalid;
    }
    // update the remaining size of the destination buffer
    dsize = bufsize - (dest - buf);
  }

  *dest++ = ']';
  *dest++ = 0;

  return true;
loc_invalid:
  return false;
}
