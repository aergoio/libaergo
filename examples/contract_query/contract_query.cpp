#include "aergo.hpp"

int main() {
  Aergo aergo("testnet-api.aergo.io", 7845);
  char response[1024];

  std::cout << "Connected. Sending request...\n";

  bool ret = aergo.query_smart_contract(
    response, sizeof response,
    "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7",
    "hello"
  );

  if (ret == true) {
    std::cout << "------------------------------------\n";
    std::cout << "Smart Contract Query OK\n";
    std::cout << "Response: " << response << "\n";
    std::cout << "------------------------------------\n";
  } else {
    std::cout << "FAILED when querying the smart contract\n";
  }

  std::cout << "Disconnected\n";
  return 0;
}
