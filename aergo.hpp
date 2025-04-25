extern "C" {
#include "aergo.h"
}

#include <iostream>
#include <sstream>
#include <string>

using namespace std;


class Aergo {
  aergo *instance;

  template<typename Arg>
  void add_json_value(stringstream &result, Arg arg){

    if (result.rdbuf()->in_avail())
      result << ",";

    if constexpr (std::is_same_v<Arg, string>) {
      result << "\"" << arg << "\"";
    } else if constexpr (std::is_same_v<Arg, const char*>) {
      result << "\"" << arg << "\"";
    } else if constexpr (std::is_same_v<Arg, char*>) {
      result << "\"" << arg << "\"";
    } else if constexpr (std::is_same_v<Arg, bool>) {
      result << std::boolalpha;
      result << arg;
      result << std::noboolalpha;
    } else if constexpr (std::is_same_v<Arg, nullptr_t>) {
      result << "null";
    } else {
      result << arg;
    }
  }

  template<typename... Args>
  std::string to_json_array(const Args&... args) {
    std::stringstream result;
    (add_json_value(result, args), ...);
    return "[" + result.str() + "]";
  }

public:
  // Constructor
  Aergo(string host, int port) {
    instance = aergo_connect(host.c_str(), port);
  }
  // Destructor
  ~Aergo() {
    aergo_free(instance);
  }

  int process_requests(int timeout = 0) {
    return aergo_process_requests(instance, timeout);
  }


  // Accounts

  bool check_privkey(aergo_account *account) {
    return aergo_check_privkey(instance, account);
  }

  bool get_account_state(aergo_account *account, char *error) {
    return aergo_get_account_state(instance, account, error);
  }


  // Transfer - synchronous

  bool transfer(transaction_receipt *receipt, aergo_account *from_account, string to_account, double value) {
    return aergo_transfer(instance, receipt, from_account, to_account.c_str(), value);
  }
  bool transfer(transaction_receipt *receipt, aergo_account *from_account, string to_account, uint64_t integer, uint64_t decimal) {
    return aergo_transfer_int(instance, receipt, from_account, to_account.c_str(), integer, decimal);
  }
  bool transfer(transaction_receipt *receipt, aergo_account *from_account, string to_account, string value) {
    return aergo_transfer_str(instance, receipt, from_account, to_account.c_str(), value.c_str());
  }
  bool transfer(transaction_receipt *receipt, aergo_account *from_account, string to_account, unsigned char *amount, int len) {
    return aergo_transfer_bignum(instance, receipt, from_account, to_account.c_str(), amount, len);
  }

  // Transfer - asynchronous

  bool transfer_async(transaction_receipt_cb cb, void *arg, aergo_account *from_account, string to_account, double value) {
    return aergo_transfer_async(instance, cb, arg, from_account, to_account.c_str(), value);
  }
  bool transfer_async(transaction_receipt_cb cb, void *arg, aergo_account *from_account, string to_account, uint64_t integer, uint64_t decimal) {
    return aergo_transfer_int_async(instance, cb, arg, from_account, to_account.c_str(), integer, decimal);
  }
  bool transfer_async(transaction_receipt_cb cb, void *arg, aergo_account *from_account, string to_account, string value) {
    return aergo_transfer_str_async(instance, cb, arg, from_account, to_account.c_str(), value.c_str());
  }
  bool transfer_async(transaction_receipt_cb cb, void *arg, aergo_account *from_account, string to_account, unsigned char *amount, int len) {
    return aergo_transfer_bignum_async(instance, cb, arg, from_account, to_account.c_str(), amount, len);
  }


  // Call smart contract function - synchronous

  bool call_smart_contract_json(transaction_receipt *receipt, aergo_account *account, string contract_address, string function, string args) {
    return aergo_call_smart_contract_json(instance, receipt, account, contract_address.c_str(), function.c_str(), args.c_str());
  }

  template<typename... Args>
  bool call_smart_contract(transaction_receipt *receipt, aergo_account *account, string contract_address, string function, Args... args) {
    string json_args = to_json_array(args...);
    return aergo_call_smart_contract_json(instance, receipt, account, contract_address.c_str(), function.c_str(), json_args.c_str());
  }

  // Call smart contract function - asynchronous

  bool call_smart_contract_json_async(transaction_receipt_cb cb, void *arg, aergo_account *account, string contract_address, string function, string args) {
    return aergo_call_smart_contract_json_async(instance, cb, arg, account, contract_address.c_str(), function.c_str(), args.c_str());
  }

  template<typename... Args>
  bool call_smart_contract_async(transaction_receipt_cb cb, void *arg, aergo_account *account, string contract_address, string function, Args... args) {
    string json_args = to_json_array(args...);
    return aergo_call_smart_contract_json_async(instance, cb, arg, account, contract_address.c_str(), function.c_str(), json_args.c_str());
  }

  // MultiCall - synchronous

  bool multicall(transaction_receipt *receipt, aergo_account *account, string payload) {
    return aergo_multicall(instance, receipt, account, payload.c_str());
  }

  // MultiCall - asynchronous

  bool multicall_async(transaction_receipt_cb cb, void *arg, aergo_account *account, string payload) {
    return aergo_multicall_async(instance, cb, arg, account, payload.c_str());
  }

  // Query smart contract - synchronous

  bool query_smart_contract_json(char *result, int resultlen, string contract_address, string function, string args) {
    return aergo_query_smart_contract_json(instance, result, resultlen, contract_address.c_str(), function.c_str(), args.c_str());
  }

  template<typename... Args>
  bool query_smart_contract(char *result, int resultlen, string contract_address, string function, Args... args) {
    string json_args = to_json_array(args...);
    return aergo_query_smart_contract_json(instance, result, resultlen, contract_address.c_str(), function.c_str(), json_args.c_str());
  }

  // Query smart contract - asynchronous

  bool query_smart_contract_json_async(query_smart_contract_cb cb, void *arg, string contract_address, string function, string args) {
    return aergo_query_smart_contract_json_async(instance, cb, arg, contract_address.c_str(), function.c_str(), args.c_str());
  }

  template<typename... Args>
  bool query_smart_contract_async(query_smart_contract_cb cb, void *arg, string contract_address, string function, Args... args) {
    string json_args = to_json_array(args...);
    return aergo_query_smart_contract_json_async(instance, cb, arg, contract_address.c_str(), function.c_str(), json_args.c_str());
  }

  // Query smart contract state variable - synchronous

  bool query_smart_contract_state_variable(char *result, int resultlen, string contract_address, string state_var) {
    return aergo_query_smart_contract_state_variable(instance, result, resultlen, contract_address.c_str(), state_var.c_str());
  }

  // Query smart contract state variable - asynchronous

  bool query_smart_contract_state_variable_async(query_state_var_cb cb, void *arg, string contract_address, string state_var) {
    return aergo_query_smart_contract_state_variable_async(instance, cb, arg, contract_address.c_str(), state_var.c_str());
  }

  // Smart contract events

  bool contract_events_subscribe(string contract_address, string event_name, contract_event_cb cb, void *arg) {
    return aergo_contract_events_subscribe(instance, contract_address.c_str(), event_name.c_str(), cb, arg);
  }


};
