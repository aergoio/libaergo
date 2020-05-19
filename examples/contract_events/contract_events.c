#include <stdio.h>
#include <stdlib.h>
#include "aergo.h"

void on_contract_event(void *arg, contract_event *event){

  puts  ("");
  puts  ("------------------------------------");
  puts  ("     Smart Contract Event");
  printf("contractAddress: %s\n", event->contractAddress);
  printf("eventName: %s\n", event->eventName);
  printf("jsonArgs: %s\n", event->jsonArgs);
  printf("eventIdx: %d\n", event->eventIdx);
  printf("blockNo: %llu\n", event->blockNo);
  printf("txIndex: %d\n", event->txIndex);
  puts  ("------------------------------------");
  puts  ("");

}

int main() {
  aergo *instance;
  char response[1024];

  instance = aergo_connect("testnet-api.aergo.io", 7845);
  if (!instance) {
    puts("Error connecting to aergo network");
    exit(1);
  }

  puts("Connected. Sending request...");

  bool ret = aergo_contract_events_subscribe(
    instance,
    "AmgMhLWDzwL2Goet6k4vxKniZksuEt3Dy8ULmiyDPpSmgJ5CgGZ4",
    "",
    on_contract_event, NULL);

  if (ret == true) {
    puts("waiting for events...");
    while (1) {
      /*
      ** instead of a loop, your application can put this call
      ** on a timer callback and use timeout = 0
      */
      aergo_process_requests(instance, 5000);
    }
  } else {
    puts("request FAILED");
  }

  aergo_free(instance);
  puts("Disconnected");

  return 0;
}
