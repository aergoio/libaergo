#include "ledger/ledger-api.c"

#define APDU_CLA   0xAE

#define APDU_INS_GET_APP_VERSION 0x01
#define APDU_INS_GET_PUBLIC_KEY  0x02
#define APDU_INS_DISPLAY_ACCOUNT 0x03
#define APDU_INS_SIGN_TXN        0x04
#define APDU_INS_SIGN_MSG        0x08

///////////////////////////////////////////////////////////////////////////////////////////////////

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

  result = ledger_send_apdu(APDU_CLA, APDU_INS_SIGN_TXN, 0, txn, txn_size, out, sizeof out, &sw, error);

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

  len = get_account_derivation_path(path, account->index);

  result = ledger_send_apdu(APDU_CLA, APDU_INS_GET_PUBLIC_KEY, 0, path, len, pubkey, sizeof pubkey, &sw, error);

  if (check_ledger_result(result, sw, error) == false) {
    return false;
  }

  memcpy(account->pubkey, pubkey, 33);
  return true;
}
