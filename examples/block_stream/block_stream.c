#include <stdio.h>
#include <stdlib.h>
#include "aergo.h"

void on_block(uint64_t block_number, uint64_t timestamp){

  puts  ("");
  puts  ("------------------------------------");
  puts  ("      New block");
  printf("height: %llu\n", block_number);
  printf("timestamp: %llu\n", timestamp);
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

  bool ret = aergo_block_stream_subscribe(instance, on_block);

  if (ret == true) {
    puts("waiting for blocks...");
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
