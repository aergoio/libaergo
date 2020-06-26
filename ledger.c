#include "dynamic-lib.c"
//#include "libaergo-ledger.h"

#define APDU_CLA   0xAE

#define APDU_INS_GET_APP_VERSION 0x01
#define APDU_INS_GET_PUBLIC_KEY  0x02
#define APDU_INS_DISPLAY_ACCOUNT 0x03
#define APDU_INS_SIGN_TXN        0x04
#define APDU_INS_SIGN_MSG        0x08

#define SW_OK      0x9000
#define SW_UNKNOWN 0x6D00

typedef int (*fn_ledger_send_apdu)(
  unsigned char apdu_cla, unsigned char apdu_ins, unsigned char apdu_p1,
  const char *data, unsigned int datalen,
  unsigned char *out, unsigned int outsize,
  int *psw
);

fn_ledger_send_apdu ledger_send_apdu = NULL;

bool load_ledger_library(char *error){
  char *zFile;

#ifdef _WIN32
  zFile = "libaergo-ledger.dll";
#elif __APPLE__
  zFile = "libaergo-ledger.dylib";
#else
  zFile = "libaergo-ledger.so";
#endif

  if (ledger_send_apdu == NULL) {
    void *h = dylib_open(zFile);
    if (h == NULL) {
      if (error) {
        char errmsg[256];
        dylib_error(sizeof errmsg, errmsg);
        snprintf(error, 256, "failed to load the libaergo-ledger library: %s", errmsg);
      }
      return false;
    }
    ledger_send_apdu = (fn_ledger_send_apdu) dylib_sym(h, "ledger_send_apdu");
    if (ledger_send_apdu == NULL) {
      if (error) {
        char errmsg[256];
        dylib_error(sizeof errmsg, errmsg);
        snprintf(error, 256, "failed to read the libaergo-ledger library: %s", errmsg);
      }
      return false;
    }
  }

  return true;
}

int ledger_get_public_key(unsigned char *path, unsigned int len, unsigned char *out, unsigned int outsize, int *psw, char *error) {

  if (load_ledger_library(error) == false) return -2;

  return ledger_send_apdu(APDU_CLA, APDU_INS_GET_PUBLIC_KEY, 0, (char*)path, len, out, outsize, psw);

}

/*
int ledger_sign_transaction(const char *txn, unsigned char *out, unsigned int outsize, int *psw, char *error) {

  if (load_ledger_library(error) == false) return -2;

  return ledger_send_apdu(APDU_CLA, APDU_INS_SIGN_TXN, 0, txn, strlen(txn), out, outsize, psw);

}
*/

bool check_ledger_result(int result, int sw, char *error){

  if (result == -1 && sw == -1) {
    if (error && error[0] == 0)
      strcpy(error, "No dongle found or application not open");
    return false;
  }
  if (result < 0) {
    if (error && error[0] == 0)
      strcpy(error, "I/O error or library not found");
    return false;
  }
  if (sw != SW_OK) {
    if (error && error[0] == 0)
      snprintf(error, 256, "Dongle application error : %.4x", sw);
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool ledger_sign_transaction(const char *txn, int txn_size, unsigned char *sig, size_t *psigsize, char *error) {
  char out[32+80+8];
  int sigsize = *psigsize;
  int result, sw;

  if (error) error[0] = 0;

  if (load_ledger_library(error) == false) return false;

  result = ledger_send_apdu(APDU_CLA, APDU_INS_SIGN_TXN, 0, txn, txn_size, out, sizeof out, &sw);

  if (result > 0) {
    DEBUG_PRINT_BUFFER("ledger txn hash", out, 32);
    memcpy(sig, out + 32, result - 32);
    *psigsize = result - 32;
  }

  return check_ledger_result(result, sw, error);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int get_account_derivation_path(unsigned char *path, int account_index){
  unsigned int ipath[5], len, i;

  // m / purpose' / coin_type' / account' / change / address_index
  // m / 44'      / 441'       / 0'       / 0      / 0
  ipath[0] = 0x8000002C;
  ipath[1] = 0x800001B9;  /* AERGO */
  ipath[2] = 0x80000000;
  ipath[3] = 0x00000000;
  ipath[4] = account_index;

  /* convert to big endian */
  len = 0;
  for(i=0; i<5; i++){
    copy_be32((uint32_t*)path+len, &ipath[i]);
    len += 4;
  }

  return len;
}

bool ledger_get_account_public_key(aergo_account *account, char *error){
  unsigned char pubkey[33 + 10] = {0};  /* additional bytes are required */
  unsigned char path[20];
  unsigned int len;
  int result, sw;

  if (error) error[0] = 0;

  len = get_account_derivation_path(path, account->index);

  result = ledger_get_public_key(path, len, pubkey, sizeof pubkey, &sw, error);

  if (check_ledger_result(result, sw, error) == false) {
    return false;
  }

  memcpy(account->pubkey, pubkey, 33);
  return true;
}
