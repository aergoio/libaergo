#include <stdio.h>
#include <stdlib.h>
#include "aergo.h"

bool arrived = false;

void on_smart_contract_result(void *arg, char *result, int len){
  puts  ("------------------------------------");
  puts  ("Smart Contract Query OK");
  printf("Response: %s\n", result);
  puts  ("------------------------------------");
  arrived = true;
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
    "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7",
    "hello", NULL
  );

  if (ret == true) {
    puts("done. waiting for response...");
    while (!arrived) {
      /*
      ** instead of a loop, your application can put this call
      ** on a timer callback and use timeout = 0
      */
      aergo_process_requests(instance, 5000);
    }
  } else {
    puts  ("FAILED when querying the smart contract");
  }

  aergo_free(instance);
  puts("Disconnected");

  return 0;
}
