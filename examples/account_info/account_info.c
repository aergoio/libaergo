#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aergo.h"

unsigned char privkey[32] = {
  0xCD, 0xFA, 0x3B, 0xF9, 0x1A, 0x2C, 0xB6, 0x9B, 0xEC, 0xB0,
  0x16, 0x7E, 0x11, 0x00, 0xF6, 0xDE, 0x77, 0xAF, 0x05, 0xD6,
  0x4E, 0xE5, 0x0C, 0x2C, 0xD4, 0xBA, 0xDD, 0x70, 0x01, 0xF0,
  0xC5, 0x4B
};

int main() {
  aergo *instance;
  aergo_account account;
  char errmsg[256];

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
  if (aergo_get_account_state(instance, &account, errmsg) == true) {
    puts("------------------------------------");
    printf("Account address: %s\n", account.address);
    printf("Account balance: %f\n", account.balance);
    printf("Account nonce: %llu\n", account.nonce);
    //printf("Account state_root: %s\n", account.state_root);
  } else {
    printf("Failed to get the account state: %s\n", errmsg);
  }

  aergo_free(instance);
  puts("Disconnected");

  return 0;
}
