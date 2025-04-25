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

  bool success = aergo_query_smart_contract_state_variable(
    instance,
    response, sizeof response,
    "AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe",
    "Name"
  );

  puts  ("------------------------------------");
  printf("Smart Contract State Variable Query %s\n", success ? "OK" : "FAILED");
  printf("Response: %s\n", response);
  puts  ("------------------------------------");

  aergo_free(instance);
  puts("Disconnected");

  return 0;
}
