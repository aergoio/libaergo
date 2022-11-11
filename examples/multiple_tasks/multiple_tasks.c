#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "aergo.h"

unsigned char privkey[32] = {
  0xDB, 0x85, 0xDD, 0x0C, 0xBA, 0x47, 0x32, 0xA1, 0x1A, 0xEB,
  0x3C, 0x7C, 0x48, 0x91, 0xFB, 0xD2, 0xFE, 0xC4, 0x5F, 0xC7,
  0x2D, 0xB3, 0x3F, 0xB6, 0x1F, 0x31, 0xEB, 0x57, 0xE7, 0x24,
  0x61, 0x76
};

bool received_event = false;

void on_contract_event(void *arg, contract_event *event){
  puts  ("------------------------------------");
  puts  ("     Smart Contract Event");
  printf("contractAddress: %s\n", event->contractAddress);
  printf("eventName: %s\n", event->eventName);
  printf("jsonArgs: %s\n", event->jsonArgs);
  printf("eventIdx: %d\n", event->eventIdx);
  printf("blockNo: %llu\n", event->blockNo);
  printf("txIndex: %d\n", event->txIndex);
  puts  ("------------------------------------");
  received_event = true;
}

bool received_receipt = false;

void on_smart_contract_result(void *arg, transaction_receipt *receipt){
  puts  ("------------------------------------");
  puts  ("Smart Contract Call Receipt:");
  printf("status: %s\n", receipt->status);
  printf("ret: %s\n", receipt->ret);
  printf("blockNo: %llu\n", receipt->blockNo);
  printf("txIndex: %d\n", receipt->txIndex);
  printf("gasUsed: %llu\n", receipt->gasUsed);
  printf("feeUsed: %f\n", receipt->feeUsed);
  puts  ("------------------------------------");
  received_receipt = true;
}

int main() {
  aergo *instance;
  aergo_account account = {0};
  char response[1024];
  char error[256];

  instance = aergo_connect("testnet-api.aergo.io", 7845);
  if (!instance) {
    puts("Error connecting to aergo network");
    exit(1);
  }

  puts("Connected. Subscribing to contract events...");

  bool ret = aergo_contract_events_subscribe(
    instance,
    "AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe",
    "",
    on_contract_event, NULL);

  if (ret == false) {
    puts("subscribe FAILED");
    return -1;
  }


  memcpy(account.privkey, privkey, 32);

  if (aergo_get_account_state(instance, &account, error) == true) {
    puts("------------------------------------");
    printf("Account address: %s\n", account.address);
    printf("Account balance: %f\n", account.balance);
    printf("Account nonce: %llu\n", account.nonce);
  } else {
    printf("Failed to get the account state: %s\n", error);
    return -1;
  }

  puts("sending transaction...");

  ret = aergo_call_smart_contract_async(instance,
      on_smart_contract_result, NULL,
      &account,
      "AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe",
      "set_name", "s", "first");

  if (ret == false) {
    puts("send first txn FAILED");
    return -1;
  }

  puts("waiting for receipt and first event...");
  while (!received_receipt && !received_event) {
    aergo_process_requests(instance, 5000);
  }

  received_receipt = false;
  received_event = false;


  puts("sending another transaction...");

  ret = aergo_call_smart_contract_async(instance,
      on_smart_contract_result, NULL,
      &account,
      "AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe",
      "set_name", "s", "second");

  if (ret == false) {
    puts("send second txn FAILED");
    return -1;
  }

  puts("waiting for receipt and second event...");
  while (!received_receipt && !received_event) {
    aergo_process_requests(instance, 5000);
  }


  aergo_free(instance);
  puts("OK. Disconnected");

  return 0;
}
