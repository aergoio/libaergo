#include <cstring>
#include "aergo.hpp"

unsigned char privkey[32] = {
  0xDB, 0x85, 0xDD, 0x0C, 0xBA, 0x47, 0x32, 0xA1, 0x1A, 0xEB,
  0x3C, 0x7C, 0x48, 0x91, 0xFB, 0xD2, 0xFE, 0xC4, 0x5F, 0xC7,
  0x2D, 0xB3, 0x3F, 0xB6, 0x1F, 0x31, 0xEB, 0x57, 0xE7, 0x24,
  0x61, 0x76
};

void on_smart_contract_result(void *arg, transaction_receipt *receipt){
  std::cout << "------------------------------------\n";
  std::cout << "done.\n";
  std::cout << "\nTransaction Receipt:\n";
  std::cout << "status: " << receipt->status << "\n";
  std::cout << "ret: " << receipt->ret << "\n";
  std::cout << "blockNo: " << receipt->blockNo << "\n";
  std::cout << "txIndex: " << receipt->txIndex << "\n";
  std::cout << "gasUsed: " << receipt->gasUsed << "\n";
  std::cout << "feeUsed: " << receipt->feeUsed << "\n";
  std::cout << "------------------------------------\n";
}

int main() {
  Aergo aergo("testnet-api.aergo.io", 7845);
  aergo_account account = {0};

  /* load the private key in the account */
  std::memcpy(account.privkey, privkey, 32);
  /* or use the account on Ledger Nano S */
  //account.use_ledger = true;
  //account.index = 0;

  /* get the account state (public key, address, balance, nonce...) */
  if (aergo.get_account_state(&account, error) == true) {
    std::cout << "------------------------------------\n";
    std::cout << "Account address: " << account.address << "\n";
    std::cout << "Account balance: " << account.balance << "\n";
    std::cout << "Account nonce:   " << account.nonce   << "\n";
    //std::cout << "Account state_root: " << account.state_root << "\n";
  } else {
    std::cout << "FAILED to get the account state: " << error << "\n";
    return 1;
  }

  std::cout << "Calling Smart Contract Function Asynchronously...\n";
  
  unsigned char script[] = {
    "["
    "[\"call\",\"AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe\",\"set_name\",\"multicall async\"],"
    "[\"call\",\"AmhQebt5N4pikoE4fZdEbaRAExoTwrLG3F5pKoGBVRbG5KeNKeoi\",\"call_func\"],"
    "[\"call\",\"AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf\",\"set\",\"1\",\"first\"],"
    "[\"call\",\"AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf\",\"set\",\"2\",\"second\"],"
    "[\"call\",\"AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf\",\"get\",\"1\"],"
    "[\"return\", \"%last result%\"]"
    "]"
  };

  std::cout << (char*)script << std::endl;

  bool ret = aergo.multicall_async(
      on_smart_contract_result, NULL,
      &account,
      (const char*)script);

  if (ret == true) {
    std::cout << "Transaction sent. Waiting for response...\n";
    while (aergo.process_requests(5000) > 0) {
      /*
      ** instead of a loop, your application can put this call
      ** on a timer callback and use timeout = 0
      */
    }
  } else {
    std::cout << "FAILED sending transaction\n";
  }

  std::cout << "Disconnected\n";
  return 0;
}
