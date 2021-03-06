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

int main() {
  aergo *instance;
  aergo_account account;

  instance = aergo_connect("testnet-api.aergo.io", 7845);
  if (!instance) {
    puts("Error connecting to aergo network");
    exit(1);
  }

  puts("Connected");

  /* load the private key in the account */
  memset(&account, 0, sizeof(aergo_account));
  memcpy(account.privkey, privkey, 32);

  /* get the account state (public key, address, balance, nonce...) */
  aergo_get_account_state(instance, &account);

  puts  ("------------------------------------");
  printf("Account address: %s\n", account.address);
  printf("Account balance: %f\n", account.balance);
  printf("Account nonce: %llu\n", account.nonce);

  puts  ("");
  puts  ("sending transaction...");

  struct transaction_receipt receipt;

  bool ret = aergo_transfer(instance,
      &receipt,
      &account,
      "AmQFpC4idVstgqhnn7ihyueadTBVBq55LzDLbK8XbzHuhMxKAQ72",
      1.5);

  if (ret == true) {
    puts  ("done.");
    puts  ("\nTransaction Receipt:");
    printf("status: %s\n", receipt.status);
    printf("ret: %s\n", receipt.ret);
    printf("blockNo: %llu\n", receipt.blockNo);
    printf("txIndex: %d\n", receipt.txIndex);
    printf("gasUsed: %llu\n", receipt.gasUsed);
    printf("feeUsed: %f\n", receipt.feeUsed);
  } else {
    puts("transfer FAILED");
  }

  aergo_free(instance);
  puts("Disconnected");

  return 0;
}
