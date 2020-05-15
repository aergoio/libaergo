#include <stdint.h>
#include <stdbool.h>

typedef struct aergo aergo;

aergo * aergo_connect(char *host, int port);
void aergo_free(aergo *instance);


// Accounts

typedef struct aergo_account aergo_account;

struct aergo_account {
  //mbedtls_ecdsa_context keypair;
  char address[64];
  uint64_t nonce;
  double balance;
  uint8_t state_root[32];
  bool is_updated;
};

int aergo_load_account_secret(aergo_account *account, char *buf, int len);
int aergo_export_account_secret(aergo_account *account, char *buf, int len);

bool aergo_get_account_state(aergo *instance, aergo_account *account);

void aergo_free_account(aergo_account *account);


// Transaction Receipt

typedef struct transaction_receipt transaction_receipt;

struct transaction_receipt {
  char contractAddress[56];  // in expanded/string form
  char status[256];
  char ret[256];  // value returned from the sc function?
  uint64_t blockNo;
  char blockHash[32];
  int32_t txIndex;
  char txHash[32];
  uint64_t gasUsed;
  double feeUsed;
  bool feeDelegation;
};

typedef void (*transaction_receipt_cb)(void *arg, transaction_receipt *receipt);

bool aergo_get_receipt(aergo *instance, char *txn_hash, struct transaction_receipt *receipt);

bool aergo_get_receipt_async(aergo *instance, char *txn_hash, transaction_receipt_cb cb, void *arg);


// Transfer - synchronous

// amount as double
bool aergo_transfer(aergo *instance, transaction_receipt *receipt, aergo_account *from_account, char *to_account, double value);

// amount as integer and decimal (note: decimal with 18 digits!)
bool aergo_transfer_int(aergo *instance, transaction_receipt *receipt, aergo_account *from_account, char *to_account, uint64_t integer, uint64_t decimal);

// amount as string (null terminated) eg: "130.25" - unit: aergo)
bool aergo_transfer_str(aergo *instance, transaction_receipt *receipt, aergo_account *from_account, char *to_account, char *value);

// amount as big-endian variable size integer
bool aergo_transfer_bignum(aergo *instance, transaction_receipt *receipt, aergo_account *from_account, char *to_account, char *amount, int len);

// Transfer - asynchronous

bool aergo_transfer_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *from_account, char *to_account, double value);

bool aergo_transfer_int_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *from_account, char *to_account, uint64_t integer, uint64_t decimal);

bool aergo_transfer_str_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *from_account, char *to_account, char *value);

bool aergo_transfer_bignum_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *from_account, char *to_account, char *amount, int len);



// Call smart contract function - synchronous

bool aergo_call_smart_contract_json(aergo *instance, transaction_receipt *receipt, aergo_account *account, char *contract_address, char *function, char *args);

bool aergo_call_smart_contract(aergo *instance, transaction_receipt *receipt, aergo_account *account, char *contract_address, char *function, char *types, ...);

  // they should return int with error

// Call smart contract function - asynchronous

bool aergo_call_smart_contract_json_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *account, char *contract_address, char *function, char *args);

bool aergo_call_smart_contract_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *account, char *contract_address, char *function, char *types, ...);

  // or maybe set the account and callback before the fn call? (to decrease the # of args)


// Query smart contract - synchronous

bool aergo_query_smart_contract_json(aergo *instance, char *result, int resultlen, char *contract_address, char *function, char *args);

bool aergo_query_smart_contract(aergo *instance, char *result, int resultlen, char *contract_address, char *function, char *types, ...);

// Query smart contract - asynchronous

typedef void (*query_smart_contract_cb)(void *arg, char *result, int len);

bool aergo_query_smart_contract_json_async(aergo *instance, query_smart_contract_cb cb, void *arg, char *contract_address, char *function, char *args);

bool aergo_query_smart_contract_async(aergo *instance, query_smart_contract_cb cb, void *arg, char *contract_address, char *function, char *types, ...);


// Smart contract events

typedef struct contract_event contract_event;

struct contract_event {
  char contractAddress[64];
  char eventName[64];
  char jsonArgs[2048];
  int32_t eventIdx;
  char txHash[32];
  char blockHash[32];
  uint64_t blockNo;
  int32_t txIndex;
};

typedef void (*contract_event_cb)(void *arg, contract_event *event);

bool aergo_contract_events_subscribe(aergo *instance, char *contract_address, char *event_name, contract_event_cb cb, void *arg);


// Blocks

bool aergo_get_block(aergo *instance, uint64_t block_number);

bool aergo_block_stream_subscribe(aergo *instance);


// Status

bool aergo_get_blockchain_status(aergo *instance);
