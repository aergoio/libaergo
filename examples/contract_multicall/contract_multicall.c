#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aergo.h"

unsigned char privkey[32] = {
  0xDB, 0x85, 0xDD, 0x0C, 0xBA, 0x47, 0x32, 0xA1, 0x1A, 0xEB,
  0x3C, 0x7C, 0x48, 0x91, 0xFB, 0xD2, 0xFE, 0xC4, 0x5F, 0xC7,
  0x2D, 0xB3, 0x3F, 0xB6, 0x1F, 0x31, 0xEB, 0x57, 0xE7, 0x24,
  0x61, 0x76
};

unsigned char script[] = {
  "["
  "[\"call\",\"AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe\",\"set_name\",\"multicall\"],"
  "[\"call\",\"AmhQebt5N4pikoE4fZdEbaRAExoTwrLG3F5pKoGBVRbG5KeNKeoi\",\"call_func\"],"
  "[\"call\",\"AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf\",\"set\",\"1\",\"first\"],"
  "[\"call\",\"AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf\",\"set\",\"2\",\"second\"],"
  "[\"call\",\"AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf\",\"get\",\"1\"],"
  "[\"return\", \"%last result%\"]"
  "]"
};

int main() {
  aergo *instance;
  aergo_account account = {0};
  char error[256];

  instance = aergo_connect("testnet-api.aergo.io", 7845);
  if (!instance) {
    puts("Error connecting to aergo network");
    exit(1);
  }

  puts("Connected");

  /* load the private key in the account */
  memcpy(account.privkey, privkey, 32);
  /* or use the account on Ledger Nano S */
  //account.use_ledger = true;
  //account.index = 0;

  /* get the account state (public key, address, balance, nonce...) */
  if (aergo_get_account_state(instance, &account, error) == true) {
    puts("------------------------------------");
    printf("Account address: %s\n", account.address);
    printf("Account balance: %f\n", account.balance);
    printf("Account nonce: %llu\n", account.nonce);
  } else {
    printf("Failed getting the account state: %s\n", error);
    return 1;
  }

  puts("Sending Multicall transaction...");
  puts(script);

  struct transaction_receipt receipt;

  bool ret = aergo_multicall(instance, &receipt, &account, script);

  if (ret == true) {
    puts("--- Transaction Receipt ---");
    printf("Status: %s\n", receipt.status);
    printf("ret: %s\n", receipt.ret);
    printf("BlockNo: %llu\n", receipt.blockNo);
    printf("TxIndex: %d\n", receipt.txIndex);
    printf("GasUsed: %llu\n", receipt.gasUsed);
    printf("FeeUsed: %f\n", receipt.feeUsed);
  } else {
    printf("Failed to send the transaction: %s\n", receipt.ret);
  }

  aergo_free(instance);
  puts("Disconnected");

  return 0;
}
