#include "aergo.hpp"

int main() {
  Aergo aergo("testnet-api.aergo.io", 7845);
  char response[1024];

  std::cout << "Connected. Sending request...\n";

  bool success = aergo.query_smart_contract_state_variable(
    response, sizeof response,
    "AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe",
    "Name"
  );

  std::cout << "------------------------------------\n";
  std::cout << "Smart Contract State Variable Query " << (success ? "OK" : "FAILED") << "\n";
  std::cout << "Response: " << response << "\n";
  std::cout << "------------------------------------\n";

  std::cout << "Disconnected\n";
  return 0;
}
