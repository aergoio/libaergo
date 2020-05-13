//#include <stdio.h>
#include "base58.h"
#include "sha256.h"
#include "account.h"

bool sha256(void *hash, const void *data, size_t len) {
  SHA256_CTX ctx;

  sha256_init(&ctx);
  sha256_update(&ctx, (const uchar*) data, len);
  sha256_final(&ctx, (uchar*)hash);

  return true;
}

bool encode_address(const void *data, size_t datasize, char *out, size_t outsize){

  b58_sha256_impl = sha256;

  return b58check_enc(out, &outsize, AddressVersion, data, datasize);

}

bool decode_address(const char *encoded, size_t encsize, void *out, size_t outsize){
  char decoded[AddressLength + 1 + 4];
  size_t decsize;
  int res;

  b58_sha256_impl = sha256;

  if (outsize < AddressLength) return false;

  decsize = sizeof(decoded);
  if (b58tobin(decoded, &decsize, encoded, encsize) == false) return false;

  res = b58check(out, decsize, encoded, encsize);
  if (res == AddressVersion) return false;

  memcpy(out, &decoded[1], AddressLength);
  return true;
}

