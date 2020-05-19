#include "aergo.hpp"

void on_contract_event(void *arg, contract_event *event){

  std::cout << "\n";
  std::cout << "------------------------------------\n";
  std::cout << "     Smart Contract Event\n";
  std::cout << "contractAddress: " << event->contractAddress << "\n";
  std::cout << "eventName: " << event->eventName << "\n";
  std::cout << "jsonArgs: " << event->jsonArgs << "\n";
  std::cout << "eventIdx: " << event->eventIdx << "\n";
  std::cout << "blockNo: " << event->blockNo << "\n";
  std::cout << "txIndex: " << event->txIndex << "\n";
  std::cout << "------------------------------------\n";
  std::cout << "\n";

}

int main() {
  Aergo aergo("testnet-api.aergo.io", 7845);

  std::cout << "Sending request...\n";

  bool ret = aergo.contract_events_subscribe(
    "AmgMhLWDzwL2Goet6k4vxKniZksuEt3Dy8ULmiyDPpSmgJ5CgGZ4",
    "",
    on_contract_event, NULL);

  if (ret == true) {
    std::cout << "done. waiting for response...\n";
    while (1) {
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
