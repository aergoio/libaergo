#include "aergo.hpp"

bool arrived = false;

void on_smart_contract_result(void *arg, bool success, char *result){
  std::cout << "------------------------------------\n";
  std::cout << "Smart Contract Query " << (success ? "OK" : "FAILED") << "\n";
  std::cout << "Response: " << result << "\n";
  std::cout << "------------------------------------\n";
  arrived = true;
}

int main() {
  Aergo aergo("testnet-api.aergo.io", 7845);

  bool ret = aergo.query_smart_contract_async(
    on_smart_contract_result, NULL,
    "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7",
    "hello"
  );

  if (ret == true) {
    std::cout << "done. waiting for response...\n";
    while (!arrived) {
      /*
      ** instead of a loop, your application can put this call
      ** on a timer callback and use timeout = 0
      */
      aergo.process_requests(5000);
    }
  } else {
    std::cout << "FAILED querying the smart contract\n";
  }

  std::cout << "Disconnected\n";
  return 0;
}
