#include <cstring>
#include "aergo.hpp"

unsigned char privkey[32] = {
  0xDB, 0x85, 0xDD, 0x0C, 0xBA, 0x47, 0x32, 0xA1, 0x1A, 0xEB,
  0x3C, 0x7C, 0x48, 0x91, 0xFB, 0xD2, 0xFE, 0xC4, 0x5F, 0xC7,
  0x2D, 0xB3, 0x3F, 0xB6, 0x1F, 0x31, 0xEB, 0x57, 0xE7, 0x24,
  0x61, 0x76
};

int main() {
  Aergo aergo("testnet-api.aergo.io", 7845);
  aergo_account account = {0};

  /* load the private key in the account */
  std::memcpy(account.privkey, privkey, 32);

  /* get the account state (public key, address, balance, nonce...) */
  if (aergo.get_account_state(&account) == true) {
    std::cout << "------------------------------------\n";
    std::cout << "Account address: " << account.address << "\n";
    std::cout << "Account balance: " << account.balance << "\n";
    std::cout << "Account nonce:   " << account.nonce   << "\n";
    //std::cout << "Account state_root: " << account.state_root << "\n";
  } else {
    std::cout << "FAILED to get the account state\n";
    return 1;
  }

  std::cout << "\nsending transaction...\n";

  struct transaction_receipt receipt;

  bool ret = aergo.transfer(
      &receipt,
      &account,
      "AmQFpC4idVstgqhnn7ihyueadTBVBq55LzDLbK8XbzHuhMxKAQ72",
      1.5);

  if (ret == true) {
    std::cout << "done.\n";
    std::cout << "\nTransaction Receipt:\n";
    std::cout << "status: " << receipt.status << "\n";
    std::cout << "ret: " << receipt.ret << "\n";
    std::cout << "blockNo: " << receipt.blockNo << "\n";
    std::cout << "txIndex: " << receipt.txIndex << "\n";
    std::cout << "gasUsed: " << receipt.gasUsed << "\n";
    std::cout << "feeUsed: " << receipt.feeUsed << "\n";
  } else {
    std::cout << "transfer FAILED\n";
  }

  std::cout << "Disconnected\n";
  return 0;
}
