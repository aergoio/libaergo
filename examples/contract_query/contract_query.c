#include <stdio.h>
#include <stdlib.h>
#include "aergo.h"

int main() {
  aergo *instance;
  char response[1024];

  instance = aergo_connect("testnet-api.aergo.io", 7845);
  if (!instance) {
    puts("Error connecting to aergo network");
    exit(1);
  }

  puts("Connected. Sending request...");

  bool ret = aergo_query_smart_contract(
    instance,
    response, sizeof response,
    "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7",
    "hello", NULL
  );

  if (ret == true) {
    puts  ("------------------------------------");
    puts  ("Smart Contract Query OK");
    printf("Response: %s\n", response);
    puts  ("------------------------------------");
  } else {
    puts  ("FAILED when querying the smart contract");
  }

  aergo_free(instance);
  puts("Disconnected");

  return 0;
}
