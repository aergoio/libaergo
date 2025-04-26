#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

#include <curl/curl.h>

#include "secp256k1-vrf.h"


#ifdef _WIN32
#define EXPORTED __declspec(dllexport)
#else
#define EXPORTED __attribute__((visibility("default")))
#endif


typedef struct request request;

typedef bool (*process_response_cb)(aergo *instance, request *request);

struct request {
  struct request *next;
  struct request *parent;
  aergo *instance;
  void *data;
  int size;
  struct aergo_account *account;
  char txn_hash[32];
  void *callback;
  void *arg;
  void *return_ptr;
  int return_size;
  process_response_cb process_response;
  process_response_cb process_error;
  char *response;
  int response_size;
  int remaining_size;
  int received;
  bool processed;
  bool success;
  char *error_msg;
  bool need_receipt;
  bool processing_receipt;
  int  receipt_attempts;
  bool keep_active;
};


// Aergo connection instance
struct aergo {
  char host[32];
  int port;
  secp256k1_context *ecdsa;
  uint8_t blockchain_id_hash[32];
  int timeout;
  request *requests;
  CURLM *multi;
  int transfers;  // num_requests
};


static request * new_request(aergo *instance);
void free_request(aergo *instance, struct request *request);


uint32_t encode_http2_data_frame(uint8_t *buffer, uint32_t content_size);

static bool aergo_get_receipt__internal(aergo *instance, const char *txn_hash, transaction_receipt_cb cb, void *arg, struct transaction_receipt *receipt, struct request *parent);
