#include <stdio.h>
#include <stdlib.h>
#include "aergo.h"

void on_smart_contract_result(void *arg, bool success, char *result){
  puts  ("------------------------------------");
  printf("Smart Contract Query %s\n", success ? "OK" : "FAILED");
  printf("Response: %s\n", result);
  puts  ("------------------------------------");
}

int main() {
  aergo *instance;

  instance = aergo_connect("testnet-api.aergo.io", 7845);
  if (!instance) {
    puts("Error connecting to aergo network");
    exit(1);
  }

  puts("Connected. Sending request...");

  bool ret = aergo_query_smart_contract_async(
    instance,
    on_smart_contract_result, NULL,
    "AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe",
    "hello", NULL
  );

  if (ret == true) {
    puts("done. waiting for response...");
    while (aergo_process_requests(instance, 5000) > 0) {
      /*
      ** instead of a loop, your application can put this call
      ** on a timer callback and use timeout = 0
      */
    }
  } else {
    puts  ("FAILED when querying the smart contract");
  }

  aergo_free(instance);
  puts("Disconnected");

  return 0;
}
