#include "aergo.h"
#include "aergo-int.h"

#include <inttypes.h>
#include "stdarg.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(DEBUG_MESSAGES)
#define DEBUG_PRINTF       printf
#define DEBUG_PRINTLN      puts
#define DEBUG_PRINT_BUFFER print_buffer

static void print_buffer(const char *title, unsigned char *data, size_t len){
  size_t i;
  DEBUG_PRINTF("%s (%zu bytes) ", title, len);
  for(i=0; i<len; i++){
    DEBUG_PRINTF(" %02x", data[i]);
    if(i % 16 == 15) DEBUG_PRINTF("\n");
  }
  DEBUG_PRINTF("\n");
}

#else
#define DEBUG_PRINTF(...)
#define DEBUG_PRINTLN(...)
#define DEBUG_PRINT_BUFFER(...)
#endif

size_t strlen2(const char *str){
  if (!str) return 0;
  return strlen(str);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#include "nanopb/pb_common.c"
#include "nanopb/pb_encode.c"
#include "nanopb/pb_decode.c"

#include "blockchain.pb.c"

#include "endianess.c"
#include "sha256.c"
#include "base58.c"
#include "mbedtls/bignum.c"
#include "conversions.c"

#include "account.c"
#include "socket.c"

#include "ledger.c"

///////////////////////////////////////////////////////////////////////////////////////////////////
// DECODING
///////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t null_byte[1] = {0};

struct blob {
  uint8_t *ptr;
  size_t  size;
};

bool print_string(pb_istream_t *stream, const pb_field_t *field, void **arg){
    uint8_t buffer[1024] = {0};

    DEBUG_PRINTF("print_string stream->bytes_left=%zu field=%p\n", stream->bytes_left, field);

    /* We could read block-by-block to avoid the large buffer... */
    if (stream->bytes_left > sizeof(buffer) - 1)
        return false;

    if (!pb_read(stream, buffer, stream->bytes_left))
        return false;

    /* Print the string, in format comparable with protoc --decode.
     * Format comes from the arg defined in main().
     */
    DEBUG_PRINTF("%s: %s\n", (char*)*arg, buffer);
    return true;
}

bool read_string(pb_istream_t *stream, const pb_field_t *field, void **arg){
    struct blob *str = *(struct blob**)arg;
    size_t len = stream->bytes_left;

    if (!str) return true;
    DEBUG_PRINTF("read_string bytes_left=%zu str->size=%zu\n", stream->bytes_left, str->size);

    /* We could read block-by-block to avoid the large buffer... */
    if (stream->bytes_left > str->size){
        DEBUG_PRINTF("FAILED! read_string\n");
        return false;
    }

    if (!pb_read(stream, str->ptr, stream->bytes_left))
        return false;

    str->ptr[len] = 0;  // null terminator

    DEBUG_PRINTF("read_string ok: %s\n", str->ptr);
    return true;
}

bool read_blob(pb_istream_t *stream, const pb_field_t *field, void **arg){
    struct blob *blob = *(struct blob**)arg;

    DEBUG_PRINTF("read_blob arg=%p\n", blob);
    if (!blob) return true;
    DEBUG_PRINTF("read_blob bytes_left=%zu blob->size=%zu\n", stream->bytes_left, blob->size);

    /* We could read block-by-block to avoid the large buffer... */
    if (stream->bytes_left > blob->size){
        DEBUG_PRINTF("FAILED! read_blob\n");
        return false;
    }

    if (!pb_read(stream, blob->ptr, stream->bytes_left))
        return false;

    DEBUG_PRINTF("read_blob ok\n");
    return true;
}

bool print_blob(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    uint8_t buffer[1024] = {0};
    int len = stream->bytes_left;
    int i;

    //DEBUG_PRINTF("print_blob stream->bytes_left=%zu field=%p\n", stream->bytes_left, field);

    /* We could read block-by-block to avoid the large buffer... */
    if (stream->bytes_left > sizeof(buffer) - 1)
        return false;

    if (!pb_read(stream, buffer, stream->bytes_left))
        return false;

    DEBUG_PRINT_BUFFER((char*)*arg, buffer, len);

    return true;
}

bool read_bignum_to_double(pb_istream_t *stream, const pb_field_t *field, void **arg){
    double *pvalue = *(double**)arg;
    uint8_t buf[64];
    size_t len = stream->bytes_left;

    DEBUG_PRINTF("read_bignum_to_double arg=%p\n", pvalue);
    if (!pvalue) return true;
    DEBUG_PRINTF("read_bignum_to_double bytes_left=%zu\n", stream->bytes_left);

    if (stream->bytes_left > sizeof(buf)){
        DEBUG_PRINTF("FAILED! read_bignum_to_double\n");
        return false;
    }

    // read the bytes
    if (!pb_read(stream, buf, stream->bytes_left))
        return false;

    // convert the value from big endian integer to string
    if (bignum_to_string(buf, len, buf, sizeof(buf)) <= 0) return false;

    DEBUG_PRINTF("read_bignum_to_double - string value = %s\n", buf);

    // convert the value from string to double
    *pvalue = atof((char*)buf);

    DEBUG_PRINTF("read_bignum_to_double ok - value = %f\n", *pvalue);
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ENCODING
///////////////////////////////////////////////////////////////////////////////////////////////////

// https://github.com/nanopb/nanopb/blob/master/tests/callbacks/encode_callbacks.c

bool encode_fixed64(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    uint64_t value = **(uint64_t**)arg;
    uint64_t value2;

    DEBUG_PRINTF("encode_fixed64 - value=%" PRIu64 "\n", value);

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    //copy_be64(&value2, &value);  // convert to big endian
    value2 = value;

    return pb_encode_fixed64(stream, &value2);
}

bool encode_varuint64(pb_ostream_t *stream, const pb_field_t *field, void * const *arg){
    uint64_t value = **(uint64_t**)arg;
    uint64_t value2;
    uint8_t *ptr;
    size_t len;

    DEBUG_PRINTF("encode_varuint64 - value=%" PRIu64 "\n", value);

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    // convert the value to big endian
    copy_be64(&value2, &value);

    // skip the null bytes, unless the last one
    ptr = (uint8_t*)&value2;
    len = 8;
    while( *ptr==0 && len>1 ){ ptr++; len--; }

    DEBUG_PRINTF("encode_varuint64 - len=%zu\n", len);

    return pb_encode_string(stream, ptr, len);
}

bool encode_account_address(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
    char *str = *(char**)arg;
    char decoded[128];
    bool res;

    DEBUG_PRINTF("encode_account_address '%s'\n", str);

    if (strlen(str) != EncodedAddressLength) {
      DEBUG_PRINTF("Lenght of address is invalid: %zu. It should be %d\n", strlen(str), EncodedAddressLength);
      return false;
    }

    res = decode_address(str, strlen(str), decoded, sizeof(decoded));
    if (!res) return false;

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, (uint8_t*)decoded, AddressLength);
}

bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
    char *str = *(char**)arg;

    DEBUG_PRINTF("encode_string '%s'\n", str);

    if (!str) return true;

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, (uint8_t*)str, strlen(str));
}

bool encode_blob(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
    struct blob *blob = *(struct blob**)arg;

    DEBUG_PRINTF("encode_blob arg=%p\n", blob);

    if (!blob || blob->ptr == NULL || blob->size == 0) return true;

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, blob->ptr, blob->size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// HTTP2 REQUEST CALLBACKS
///////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t blockchain_id_hash[32] = {0};

bool handle_blockchain_status_response(aergo *instance, struct request *request) {
  char *data = request->response;
  int len = request->received;
  BlockchainStatus status = BlockchainStatus_init_zero;

  DEBUG_PRINT_BUFFER("handle_blockchain_status_response", data, len);

  /* Create a stream that reads from the buffer */
  pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

  /* Set the callback functions */
  struct blob bb = { .ptr = blockchain_id_hash, .size = 32 };
  status.best_chain_id_hash.arg = &bb;
  status.best_chain_id_hash.funcs.decode = &read_blob;

  /* Decode the message */
  if (pb_decode(&stream, BlockchainStatus_fields, &status) == false) {
    DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  /* Print the data contained in the message */
  DEBUG_PRINT_BUFFER("ChainIdHash: ", blockchain_id_hash, sizeof(blockchain_id_hash));

  return true;
}

bool handle_account_state_response(aergo *instance, struct request *request) {
  char *data = request->response;
  int len = request->received;
  State account_state = State_init_zero;

  DEBUG_PRINT_BUFFER("handle_account_state_response", data, len);

  /* Create a stream that reads from the buffer */
  pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

  /* Set the callback functions */

  account_state.balance.arg = &request->account->balance;
  account_state.balance.funcs.decode = &read_bignum_to_double;

  struct blob bb = { .ptr = request->account->state_root, .size = 32 };
  account_state.storageRoot.arg = &bb;
  account_state.storageRoot.funcs.decode = &read_blob;

  /* Decode the message */
  if (pb_decode(&stream, State_fields, &account_state) == false) {
    DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  request->account->is_updated = true;
  request->account->nonce = account_state.nonce;

  /* Print the data contained in the message */
  DEBUG_PRINTF("Account Nonce: %" PRIu64 "\n", account_state.nonce);

  return true;
}

bool handle_block_response(aergo *instance, struct request *request) {
  char *data = request->response;
  int len = request->received;
  Block block = Block_init_zero;

  DEBUG_PRINT_BUFFER("handle_block_response", data, len);

  /* Create a stream that reads from the buffer */
  pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

  /* Set the callback functions */
  block.header.chainID.funcs.decode = &print_blob;
  block.header.chainID.arg = (void*)"chainID";
  block.header.pubKey.funcs.decode = &print_blob;
  block.header.pubKey.arg = (void*)"pubKey";
  block.body.txs.funcs.decode = &print_string;
  block.body.txs.arg = (void*)"txs";

  /* Decode the message */
  if (pb_decode(&stream, Block_fields, &block) == false) {
    DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  /* Print the data contained in the message */
  DEBUG_PRINTF("Block number: %" PRIu64 "\n", block.header.blockNo);
  DEBUG_PRINTF("Block timestamp: %" PRIu64 "\n", block.header.timestamp);
  DEBUG_PRINTF("Block confirms: %" PRIu64 "\n", block.header.confirms);

  return true;
}

bool handle_transfer_response(aergo *instance, struct request *request) {
  char *data = request->response;
  int len = request->received;
  char error_msg[256];
  CommitResultList response = CommitResultList_init_zero;

  DEBUG_PRINT_BUFFER("handle_transfer_response", data, len);

  /* Create a stream that reads from the buffer */
  pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

  /* Set the callback functions */
  struct blob s1 = { .ptr = (uint8_t*) error_msg, .size = 256 };
  response.results.detail.arg = &s1;
  response.results.detail.funcs.decode = read_string;

  /* Decode the message */
  if (pb_decode(&stream, CommitResultList_fields, &response) == false) {
    DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  /* Print the data contained in the message */
  DEBUG_PRINTF("response error status: %u\n", response.results.error);

  /* was the transaction OK? */
  if (response.results.error == CommitStatus_TX_OK) {
    /* request a transaction receipt */
    return aergo_get_receipt__int(instance,
              request->txn_hash,
              request->callback,
              request->arg,
              (transaction_receipt *) request->return_ptr,
              true);
  } else {
    transaction_receipt *receipt = (transaction_receipt *) request->return_ptr;
    if (receipt) {
      strcpy(receipt->status, "FAILED");
      snprintf(receipt->ret, sizeof(receipt->ret), "error %d: %s",
          response.results.error, error_msg);
    }
  }

  // xx = response.results.error;
  return false;
}

bool handle_contract_call_response(aergo *instance, struct request *request) {
  char *data = request->response;
  int len = request->received;
  char error_msg[256];
  CommitResultList response = CommitResultList_init_zero;

  DEBUG_PRINT_BUFFER("handle_contract_call_response", data, len);

  /* Create a stream that reads from the buffer */
  pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

  /* Set the callback functions */
  struct blob s1 = { .ptr = (uint8_t*) error_msg, .size = 256 };
  response.results.detail.arg = &s1;
  response.results.detail.funcs.decode = read_string;

  /* Decode the message */
  if (pb_decode(&stream, CommitResultList_fields, &response) == false) {
    DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  /* Print the data contained in the message */
  DEBUG_PRINTF("response error status: %u\n", response.results.error);

  /* was the transaction OK? */
  if (response.results.error == CommitStatus_TX_OK) {
    /* request a transaction receipt */
    return aergo_get_receipt__int(instance,
              request->txn_hash,
              request->callback,
              request->arg,
              (transaction_receipt *) request->return_ptr,
              true);
  } else {
    transaction_receipt *receipt = (transaction_receipt *) request->return_ptr;
    if (receipt) {
      strcpy(receipt->status, "FAILED");
      snprintf(receipt->ret, sizeof(receipt->ret), "error %d: %s",
          response.results.error, error_msg);
    }
  }

  // xx = response.results.error;
  return false;
}

bool handle_query_error(aergo *instance, struct request *request) {

  DEBUG_PRINTF("handle_query_error - %s\n", request->error_msg);

  if (request->callback) {
    query_smart_contract_cb callback = (query_smart_contract_cb) request->callback;
    callback(request->arg, false, request->error_msg);
  } else {
    int size = strlen(request->error_msg);
    if (size > request->return_size) {
      size = request->return_size - 1;
    }
    memcpy(request->return_ptr, request->error_msg, size);
    ((char*)request->return_ptr)[size] = 0;
  }

  return false;  /* confirms that it was a failure */
}

bool handle_query_response(aergo *instance, struct request *request) {
  char *data = request->response;
  int len = request->received;
  SingleBytes response = SingleBytes_init_zero;

  DEBUG_PRINT_BUFFER("handle_query_response", data, len);

  if (request->callback) {
    //assert(request->return_ptr == NULL);
    //assert(request->return_size == 0);
    request->return_size = len;
    request->return_ptr = malloc(request->return_size);
    if (!request->return_ptr) return false;
  }

  /* Create a stream that reads from the buffer */
  pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

  /* Set the callback functions */
  struct blob s1 = { .ptr = (uint8_t*) request->return_ptr, .size = request->return_size };
  response.value.arg = &s1;
  response.value.funcs.decode = &read_string;

  /* Decode the message */
  if (pb_decode(&stream, SingleBytes_fields, &response) == false) {
    DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  if (request->callback) {
    query_smart_contract_cb callback = (query_smart_contract_cb) request->callback;
    callback(request->arg, true, request->return_ptr);
    free(request->return_ptr);
    request->return_ptr = NULL;
  }

  return true;
}

bool handle_event_response(aergo *instance, struct request *request) {
  char *data = request->response;
  int len = request->received;
  Event response = Event_init_zero;
  contract_event event = {0};
  char raw_address[64];

  DEBUG_PRINT_BUFFER("handle_event_response", data, len);

  /* Create a stream that reads from the buffer */
  pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

  /* Set the callback functions */

  struct blob s1 = { .ptr = (uint8_t*) raw_address, .size = sizeof raw_address };
  response.contractAddress.arg = &s1;
  response.contractAddress.funcs.decode = &read_blob;

  struct blob s2 = { .ptr = (uint8_t*) event.eventName, .size = sizeof event.eventName };
  response.eventName.arg = &s2;
  response.eventName.funcs.decode = &read_string;

  struct blob s3 = { .ptr = (uint8_t*) event.jsonArgs, .size = sizeof event.jsonArgs };
  response.jsonArgs.arg = &s3;
  response.jsonArgs.funcs.decode = &read_string;

  struct blob b1 = { .ptr = (uint8_t*) event.txHash, .size = sizeof event.txHash };
  response.txHash.arg = &b1;
  response.txHash.funcs.decode = &read_blob;

  struct blob b2 = { .ptr = (uint8_t*) event.blockHash, .size = sizeof event.blockHash };
  response.blockHash.arg = &b2;
  response.blockHash.funcs.decode = &read_blob;

  /* Decode the message */
  if (pb_decode(&stream, Event_fields, &response) == false) {
    DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  /* encode the contract address from binary to string format */
  encode_address(raw_address, AddressLength, event.contractAddress, sizeof event.contractAddress);

  /* copy values */
  event.eventIdx = response.eventIdx;
  event.blockNo = response.blockNo;
  event.txIndex = response.txIndex;

  /* Call the callback function */
  contract_event_cb callback = request->callback;
  callback(request->arg, &event);

  return true;
}

bool handle_receipt_response(aergo *instance, struct request *request) {
  char *data = request->response;
  int len = request->received;
  Receipt response = Receipt_init_zero;
  transaction_receipt *receipt, on_stack_receipt;
  char raw_address[64];

  DEBUG_PRINT_BUFFER("handle_receipt_response", data, len);

  if (request->callback) {
    receipt = &on_stack_receipt;
  } else {
    receipt = (transaction_receipt *) request->return_ptr;
  }
  memset(receipt, 0, sizeof(struct transaction_receipt));

  /* Create a stream that reads from the buffer */
  pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

  /* Set the callback functions */

  struct blob s1 = { .ptr = (uint8_t*) raw_address, .size = sizeof raw_address };
  response.contractAddress.arg = &s1;
  response.contractAddress.funcs.decode = &read_blob;

  struct blob s2 = { .ptr = (uint8_t*) receipt->status, .size = sizeof receipt->status };
  response.status.arg = &s2;
  response.status.funcs.decode = &read_string;

  struct blob s3 = { .ptr = (uint8_t*) receipt->ret, .size = sizeof receipt->ret };
  response.ret.arg = &s3;
  response.ret.funcs.decode = &read_string;

  struct blob b1 = { .ptr = (uint8_t*) receipt->txHash, .size = sizeof receipt->txHash };
  response.txHash.arg = &b1;
  response.txHash.funcs.decode = &read_blob;

  struct blob b2 = { .ptr = (uint8_t*) receipt->blockHash, .size = sizeof receipt->blockHash };
  response.blockHash.arg = &b2;
  response.blockHash.funcs.decode = &read_blob;

  response.feeUsed.arg = &receipt->feeUsed;
  response.feeUsed.funcs.decode = &read_bignum_to_double;

  /* Decode the message */
  if (pb_decode(&stream, Receipt_fields, &response) == false) {
    DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  /* encode the contract address from binary to string format */
  encode_address(raw_address, AddressLength, receipt->contractAddress, sizeof receipt->contractAddress);

  /* copy values */
  receipt->gasUsed = response.gasUsed;
  receipt->blockNo = response.blockNo;
  receipt->txIndex = response.txIndex;
  receipt->feeDelegation = response.feeDelegation;

  if (request->callback) {
    transaction_receipt_cb callback = (transaction_receipt_cb) request->callback;
    callback(request->arg, receipt);
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// AERGO SPECIFIC
///////////////////////////////////////////////////////////////////////////////////////////////////

struct txn {
  uint64_t nonce;
  unsigned char account[AddressLength];      // decoded account address
  unsigned char recipient[AddressLength];    // decoded account address
  uint8_t *amount;              // variable-length big-endian integer
  int amount_len;
  char *payload;
  uint64_t gasLimit;
  uint64_t gasPrice;            // variable-length big-endian integer
  uint32_t type;
  unsigned char *chainIdHash;   // hash value of chain identifier in the block
  unsigned char sign[80];       // sender's signature for this TxBody
  size_t sig_len;
  unsigned char hash[32];       // hash of the whole transaction including the signature
};

bool calculate_tx_hash(struct txn *txn, unsigned char *hash, bool include_signature);

bool encode_transaction_body(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
  struct txn *txn = *(struct txn **)arg;
  TxBody txbody = TxBody_init_zero;
  bool encode_as_submessage = txn->sig_len > 0;
  bool status;

  DEBUG_PRINTLN("encode_transaction_body");

  if (encode_as_submessage) {
    if (!pb_encode_tag_for_field(stream, field))
        return false;
  }

  txbody.type = (TxType) txn->type;
  txbody.nonce = txn->nonce;

  struct blob acc = { .ptr = txn->account, .size = AddressLength };
  txbody.account.arg = &acc;
  txbody.account.funcs.encode = encode_blob;

  struct blob rec = { .ptr = txn->recipient, .size = AddressLength };
  txbody.recipient.arg = &rec;
  txbody.recipient.funcs.encode = encode_blob;

  txbody.payload.arg = txn->payload;
  txbody.payload.funcs.encode = &encode_string;

  struct blob amt = { .ptr = txn->amount, .size = txn->amount_len };
  txbody.amount.arg = &amt;
  txbody.amount.funcs.encode = &encode_blob;

  txbody.gasLimit = txn->gasLimit;

  txbody.gasPrice.arg = &txn->gasPrice;
  txbody.gasPrice.funcs.encode = &encode_varuint64;

  struct blob cid = { .ptr = txn->chainIdHash, .size = 32 };
  txbody.chainIdHash.arg = &cid;
  txbody.chainIdHash.funcs.encode = &encode_blob;

  struct blob sig = { .ptr = txn->sign, .size = txn->sig_len };
  txbody.sign.arg = &sig;
  txbody.sign.funcs.encode = &encode_blob;

  /* Encode the message */
  if (encode_as_submessage) {
    status = pb_encode_submessage(stream, TxBody_fields, &txbody);
  } else {
    status = pb_encode(stream, TxBody_fields, &txbody);
  }
  if (!status) {
    DEBUG_PRINTF("Encoding failed: %s\n", PB_GET_ERROR(stream));
  }
  return status;
}

bool encode_1_transaction(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
  struct txn *txn = *(struct txn **)arg;
  Tx message = Tx_init_zero;

  DEBUG_PRINTLN("encode_1_transaction");

  if (!pb_encode_tag_for_field(stream, field))
      return false;

  calculate_tx_hash(txn, txn->hash, true);

  /* Set the values and the encoder callback functions */
  struct blob bb = { .ptr = txn->hash, .size = 32 };
  message.hash.arg = &bb;
  message.hash.funcs.encode = &encode_blob;

  message.body.arg = txn;
  message.body.funcs.encode = &encode_transaction_body;

  /* Encode the message */
  bool status = pb_encode_submessage(stream, Tx_fields, &message);
  if (!status) {
    DEBUG_PRINTF("Encoding failed: %s\n", PB_GET_ERROR(stream));
  }
  return status;
}

bool encode_transaction(uint8_t *buffer, size_t *psize, char *txn_hash, struct txn *txn, char *error) {
  TxList message = TxList_init_zero;
  uint32_t size;

  DEBUG_PRINTLN("encode_transaction");

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Set the values and the encoder callback functions */
  message.txs.arg = txn;
  message.txs.funcs.encode = &encode_1_transaction;

  /* Decode the message */
  bool status = pb_encode(&stream, TxList_fields, &message);
  if (!status) {
    snprintf(error, 256, "transaction encoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  if (txn_hash) {
    memcpy(txn_hash, txn->hash, 32);
  }

  size = encode_http2_data_frame(buffer, stream.bytes_written);

  DEBUG_PRINT_BUFFER("Message", buffer, size);

  *psize = size;
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool encode_bigint(uint8_t **pptr, uint64_t value){

  // TODO: encode the gasPrice

  //len = xxx(txn->amount);
  //memcpy(ptr, txn->amount, len); ptr += len;

  // for now it is just including a single byte (zero)
  // this should be removed:
  memcpy(*pptr, &value, 1); (*pptr)++;

  return true;
}

bool calculate_tx_hash(struct txn *txn, unsigned char *hash, bool include_signature){
  uint8_t buf[4096], *ptr;
  size_t len = 0;

  DEBUG_PRINTLN("calculate_tx_hash");

  ptr = buf;

  memcpy(ptr, &txn->nonce, 8); ptr += 8;

  memcpy(ptr, txn->account, AddressLength); ptr += AddressLength;

  //if (txn->recipient){
    memcpy(ptr, txn->recipient, AddressLength); ptr += AddressLength;
  //}

  // it must be at least 1 byte in size!
  memcpy(ptr, txn->amount, txn->amount_len); ptr += txn->amount_len;

  if (txn->payload){
    len = strlen(txn->payload);
    memcpy(ptr, txn->payload, len); ptr += len;
  }

  memcpy(ptr, &txn->gasLimit, 8); ptr += 8;

  encode_bigint(&ptr, txn->gasPrice);

  memcpy(ptr, &txn->type, 4); ptr += 4;

  memcpy(ptr, txn->chainIdHash, 32); ptr += 32;

  if (include_signature) {
    memcpy(ptr, txn->sign, txn->sig_len); ptr += txn->sig_len;
  }

  len = ptr - buf;
  sha256(hash, buf, len);

  DEBUG_PRINT_BUFFER("txn hash", hash, 32);

  return true;
}

bool sign_transaction(aergo *instance, aergo_account *account, struct txn *txn, char *error){

  DEBUG_PRINTLN("sign_transaction");

  if (account->use_ledger) {
    char txn_data[64 * 1024];
    int txn_size;
    /* encode the transaction */
    pb_ostream_t stream = pb_ostream_from_buffer(&txn_data[1], sizeof(txn_data) - 1);
    if (encode_transaction_body(&stream, NULL, (void**)&txn) == false) {
      strcpy(error, "failed to encode the transaction");
      goto loc_failed;
    }
    txn_data[0] = txn->type;
    txn_size = stream.bytes_written + 1;
    DEBUG_PRINT_BUFFER("Transaction", txn_data, txn_size);
    /* select the account on the device */
    //if (ledger_get_account_public_key(account, error) == false) {
    //  goto loc_failed;
    //}
    /* send the transaction to be signed */
    txn->sig_len = sizeof(txn->sign);
    DEBUG_PRINTLN("signing transaction on ledger...");
    if (ledger_sign_transaction(txn_data, txn_size, txn->sign, &txn->sig_len, error) == false) {
      goto loc_failed;
    }
  } else {
    secp256k1_ecdsa_signature sig;
    uint8_t hash[32];
    bool ret;
    /* calculate the transaction hash */
    calculate_tx_hash(txn, hash, false);
    DEBUG_PRINT_BUFFER("  hash", hash, sizeof(hash));
    /* sign the transaction hash */
    ret = secp256k1_ecdsa_sign(instance->ecdsa, &sig, hash, account->privkey, NULL, NULL);
    if (ret == false) {
      strcpy(error, "failed to sign the transaction");
      goto loc_failed;
    }
#if 0
    ret = secp256k1_ecdsa_signature_serialize_compact(instance->ecdsa, txn->sign, &sig);
    if (ret == false) goto loc_failed;
    txn->sig_len = 64;  /* size of the compact signature format */
#endif
    txn->sig_len = sizeof(txn->sign);
    ret = secp256k1_ecdsa_signature_serialize_der(instance->ecdsa, txn->sign, &txn->sig_len, &sig);
    if (ret == false) {
      strcpy(error, "failed to serialize the signature");
      goto loc_failed;
    }
  }

  DEBUG_PRINT_BUFFER("signature", txn->sign, txn->sig_len);
  return true;
loc_failed:
  DEBUG_PRINTF("sign_transaction: %s\n", error);
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// COMMAND ENCODING
///////////////////////////////////////////////////////////////////////////////////////////////////

bool EncodeTransfer(uint8_t *buffer, size_t *psize, char *txn_hash, aergo * instance, aergo_account *account, const char *recipient, const uint8_t *amount, int amount_len, char *error) {
  struct txn txn = {0};
  char out[64]={0};

  DEBUG_PRINTLN("EncodeTransfer");

  if (!amount || amount_len < 1) return false;

  /* increment the account nonce */
  account->nonce++;

  txn.nonce = account->nonce;
  memcpy(txn.account, account->pubkey, sizeof txn.account);
  decode_address(recipient, strlen(recipient), txn.recipient, sizeof(txn.recipient));
  txn.amount = amount;   // variable-length big-endian integer
  txn.amount_len = amount_len;
  txn.payload = NULL;
  txn.gasLimit = 0;
  txn.gasPrice = 0;      // variable-length big-endian integer
  txn.type = TxType_TRANSFER;
  txn.chainIdHash = blockchain_id_hash;

  encode_address(txn.account, sizeof txn.account, out, sizeof out);
  DEBUG_PRINTF("account address: %s\n", out);
  DEBUG_PRINTF("account nonce: %" PRIu64 "\n", account->nonce);

  if (sign_transaction(instance, account, &txn, error) == false) {
    return false;
  }

  return encode_transaction(buffer, psize, txn_hash, &txn, error);
}

bool EncodeContractCall(uint8_t *buffer, size_t *psize, char *txn_hash, const char *contract_address, const char *call_info, aergo *instance, aergo_account *account, char *error) {
  struct txn txn = {0};

  DEBUG_PRINTLN("EncodeContractCall");

  /* increment the account nonce */
  account->nonce++;

  txn.nonce = account->nonce;
  memcpy(txn.account, account->pubkey, sizeof txn.account);
  decode_address(contract_address, strlen(contract_address), txn.recipient, sizeof(txn.recipient));
  txn.amount = null_byte;   // variable-length big-endian integer
  txn.amount_len = 1;
  txn.payload = call_info;
  txn.gasLimit = 0;
  txn.gasPrice = 0;          // variable-length big-endian integer
  txn.type = TxType_CALL;
  txn.chainIdHash = blockchain_id_hash;

#ifdef DEBUG_MESSAGES
  {
  char out[64]={0};
  encode_address(txn.account, sizeof txn.account, out, sizeof out);
  DEBUG_PRINTF("account address: %s\n", out);
  DEBUG_PRINTF("account nonce: %" PRIu64 "\n", account->nonce);
  }
#endif

  if (sign_transaction(instance, account, &txn, error) == false) {
    return false;
  }

  return encode_transaction(buffer, psize, txn_hash, &txn, error);
}

bool EncodeQuery(uint8_t *buffer, size_t *psize, const char *contract_address, const char *query_info){
  Query message = Query_init_zero;
  uint32_t size;

  DEBUG_PRINTLN("EncodeQuery");

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Set the callback functions */
  message.contractAddress.funcs.encode = &encode_account_address;
  message.contractAddress.arg = contract_address;
  message.queryinfo.funcs.encode = &encode_string;
  message.queryinfo.arg = query_info;

  /* Decode the message */
  bool status = pb_encode(&stream, Query_fields, &message);
  if (!status) {
    DEBUG_PRINTF("Encoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  size = encode_http2_data_frame(buffer, stream.bytes_written);

  DEBUG_PRINT_BUFFER("Message", buffer, size);

  *psize = size;
  return true;
}

bool EncodeFilterInfo(uint8_t *buffer, size_t *psize, const char *contract_address, const char *event_name){
  FilterInfo message = FilterInfo_init_zero;
  uint32_t size;

  DEBUG_PRINTLN("EncodeFilterInfo");

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Set encoding callback functions for variable length data */
  message.contractAddress.funcs.encode = &encode_account_address;
  message.contractAddress.arg = contract_address;
  if (event_name) {
    message.eventName.funcs.encode = &encode_string;
    message.eventName.arg = event_name;
  }
#if 0
  message.argFilter.funcs.encode = &encode_string;
  message.argFilter.arg = arg_filter;
#endif

  /* Set values directly */
  message.desc = true;
#if 0
  message.blockfrom = x;
  message.blockto = y;
  message.recentBlockCnt = z;
#endif

  /* Encode the message on the buffer */
  bool status = pb_encode(&stream, FilterInfo_fields, &message);
  if (!status) {
    DEBUG_PRINTF("Encoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  size = encode_http2_data_frame(buffer, stream.bytes_written);

  DEBUG_PRINT_BUFFER("Message", buffer, size);

  *psize = size;
  return true;
}

bool EncodeAccountAddress(uint8_t *buffer, size_t *psize, aergo_account *account) {
  SingleBytes message = SingleBytes_init_zero;
  uint32_t size;

  DEBUG_PRINTLN("EncodeAccountAddress");

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Set the callback functions */
  struct blob bb = { .ptr = account->pubkey, .size = 33 };
  message.value.arg = &bb;
  message.value.funcs.encode = &encode_blob;

  /* Decode the message */
  bool status = pb_encode(&stream, SingleBytes_fields, &message);
  if (!status) {
    DEBUG_PRINTF("Encoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  size = encode_http2_data_frame(buffer, stream.bytes_written);

  DEBUG_PRINT_BUFFER("Message", buffer, size);

  *psize = size;
  return true;
}

bool EncodeTxnHash(uint8_t *buffer, size_t *psize, const char *txn_hash){
  SingleBytes message = SingleBytes_init_zero;
  uint32_t size;

  DEBUG_PRINTLN("EncodeTxnHash");

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Set the callback functions */
  struct blob bb = { .ptr = txn_hash, .size = 32 };
  message.value.arg = &bb;
  message.value.funcs.encode = &encode_blob;

  /* Decode the message */
  bool status = pb_encode(&stream, SingleBytes_fields, &message);
  if (!status) {
    DEBUG_PRINTF("Encoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  size = encode_http2_data_frame(buffer, stream.bytes_written);

  DEBUG_PRINT_BUFFER("Message", buffer, size);

  *psize = size;
  return true;
}

bool EncodeBlockNo(uint8_t *buffer, size_t *psize, uint64_t blockNo){
  SingleBytes message = SingleBytes_init_zero;
  //  BlockMetadata blockmeta = BlockMetadata_init_zero;
  //  Block block = Block_init_zero;
  uint32_t size;

  DEBUG_PRINTLN("EncodeBlockNo");

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Set the callback functions */
  message.value.funcs.encode = &encode_fixed64;
  message.value.arg = &blockNo;

  /* Decode the message */
  bool status = pb_encode(&stream, SingleBytes_fields, &message);
  if (!status) {
    DEBUG_PRINTF("Encoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  size = encode_http2_data_frame(buffer, stream.bytes_written);

  DEBUG_PRINT_BUFFER("Message", buffer, size);

  *psize = size;
  return true;
}

bool EncodeEmptyMessage(uint8_t *buffer, size_t *psize){
  Empty message = Empty_init_zero;
  uint32_t size;

  DEBUG_PRINTLN("EncodeEmptyMessage");

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Decode the message */
  bool status = pb_encode(&stream, Empty_fields, &message);
  if (!status) {
    DEBUG_PRINTF("Encoding failed: %s\n", PB_GET_ERROR(&stream));
    return false;
  }

  size = encode_http2_data_frame(buffer, stream.bytes_written);

  DEBUG_PRINT_BUFFER("Message", buffer, size);

  *psize = size;
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool check_blockchain_id_hash(aergo *instance) {

  if (blockchain_id_hash[0] == 0) {
    aergo_get_blockchain_status(instance);
  }

  return (blockchain_id_hash[0] != 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static request * new_request(aergo *instance) {
  struct request *request;

  if (!instance) return NULL;

  request = malloc(sizeof(struct request));
  if (!request) return NULL;

  memset(request, 0, sizeof(struct request));
  request->sock = INVALID_SOCKET;

  llist_add(&instance->requests, request);

  return request;
}

void free_request(aergo *instance, struct request *request) {

  if (!instance || !request) return;

  if (request->response) free(request->response);

  llist_remove(&instance->requests, request);

  free(request);

}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EXPORTED FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

static bool aergo_transfer_bignum__int(aergo *instance, transaction_receipt_cb cb, void *arg, transaction_receipt *receipt, aergo_account *from_account, const char *to_account, const char *amount, int len){
  uint8_t buffer[1024], txn_hash[32];
  size_t size;
  struct request *request = NULL;
  bool status = false;
  char error[256];

  DEBUG_PRINTLN("aergo_transfer");

  if (!instance || !from_account || !to_account || !amount || len <= 0) return false;

  if (check_blockchain_id_hash(instance) == false) return false;

  // check if nonce was retrieved
  if (!from_account->is_updated) {
    if (aergo_get_account_state(instance, from_account, error) == false) {
      if (receipt) {
        strcpy(receipt->status, "FAILED");
        strcpy(receipt->ret, error);
      }
      return false;
    }
  }

  size = sizeof(buffer);
  if (EncodeTransfer(buffer, &size, txn_hash, instance,
                     from_account, to_account, amount, len, error) == false) {
    if (receipt) {
      strcpy(receipt->status, "FAILED");
      strcpy(receipt->ret, error);
    }
    return false;
  }

  request = new_request(instance);
  if (!request) return false;

  request->data = buffer;
  request->size = size;
  memcpy(request->txn_hash, txn_hash, 32);
  request->callback = cb;
  request->arg = arg;
  request->return_ptr = receipt;

  return send_grpc_request(instance, "CommitTX", request, handle_transfer_response);
}

EXPORTED bool aergo_transfer_bignum(aergo *instance, transaction_receipt *receipt, aergo_account *from_account, const char *to_account, const unsigned char *amount, int len){
  //if (!receipt) return false; -- do not ask for a txn receipt
  return aergo_transfer_bignum__int(instance, NULL, NULL, receipt, from_account, to_account, amount, len);
}

EXPORTED bool aergo_transfer_bignum_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *from_account, const char *to_account, const unsigned char *amount, int len){
  if (!cb) return false;
  return aergo_transfer_bignum__int(instance, cb, arg, NULL, from_account, to_account, amount, len);
}

EXPORTED bool aergo_transfer_str(aergo *instance, transaction_receipt *receipt, aergo_account *from_account, const char *to_account, const char *value){
  char buf[16];
  int len;

  len = string_to_bignum(value, strlen2(value), buf, sizeof(buf));

  return aergo_transfer_bignum(instance, receipt, from_account, to_account, buf, len);
}

EXPORTED bool aergo_transfer_str_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *from_account, const char *to_account, const char *value){
  char buf[16];
  int len;

  len = string_to_bignum(value, strlen2(value), buf, sizeof(buf));

  return aergo_transfer_bignum_async(instance, cb, arg, from_account, to_account, buf, len);
}

EXPORTED bool aergo_transfer(aergo *instance, transaction_receipt *receipt, aergo_account *from_account, const char *to_account, double value){
  char amount_str[36];

  snprintf(amount_str, sizeof(amount_str), "%f", value);

  return aergo_transfer_str(instance, receipt, from_account, to_account, amount_str);
}

EXPORTED bool aergo_transfer_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *from_account, const char *to_account, double value){
  char amount_str[36];

  snprintf(amount_str, sizeof(amount_str), "%f", value);

  return aergo_transfer_str_async(instance, cb, arg, from_account, to_account, amount_str);
}

EXPORTED bool aergo_transfer_int(aergo *instance, transaction_receipt *receipt, aergo_account *from_account, const char *to_account, uint64_t integer, uint64_t decimal){
  char amount_str[36];

  snprintf(amount_str, sizeof(amount_str),
           "%" PRIu64 ".%018" PRIu64, integer, decimal);

  return aergo_transfer_str(instance, receipt, from_account, to_account, amount_str);
}

EXPORTED bool aergo_transfer_int_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *from_account, const char *to_account, uint64_t integer, uint64_t decimal){
  char amount_str[36];

  snprintf(amount_str, sizeof(amount_str),
           "%" PRIu64 ".%018" PRIu64, integer, decimal);

  return aergo_transfer_str_async(instance, cb, arg, from_account, to_account, amount_str);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static bool aergo_call_smart_contract__int(aergo *instance, transaction_receipt_cb cb, void *arg, transaction_receipt *receipt, aergo_account *account, const char *contract_address, const char *function, const char *args){
  uint8_t *call_info=NULL, *buffer=NULL, txn_hash[32];
  size_t size;
  struct request *request = NULL;
  bool status = false;
  char error[256];

  DEBUG_PRINTLN("aergo_call_smart_contract");

  if (!instance || !account || !contract_address || !function) return false;

  if (check_blockchain_id_hash(instance) == false) return false;

  // check if nonce was retrieved
  if (!account->is_updated) {
    if (aergo_get_account_state(instance, account, error) == false) {
      if (receipt) {
        strcpy(receipt->status, "FAILED");
        strcpy(receipt->ret, error);
      }
      return false;
    }
  }

  size = strlen(function) + strlen2(args);
  call_info = malloc(size + 32);
  buffer = malloc(size + 300);
  if (!call_info || !buffer) goto loc_exit;

  if (args && strlen(args) > 0) {
    snprintf(call_info, size + 32,
            "{\"Name\":\"%s\",\"Args\":%s}", function, args);
  } else {
    snprintf(call_info, size + 32,
            "{\"Name\":\"%s\"}", function);
  }

  size += 300;
  if (EncodeContractCall(buffer, &size, txn_hash, contract_address,
                         call_info, instance, account, error) == false) {
    if (receipt) {
      strcpy(receipt->status, "FAILED");
      strcpy(receipt->ret, error);
    }
    goto loc_exit;
  }

  request = new_request(instance);
  if (!request) goto loc_exit;

  request->data = buffer;
  request->size = size;
  memcpy(request->txn_hash, txn_hash, 32);
  request->callback = cb;
  request->arg = arg;
  request->return_ptr = receipt;

  status = send_grpc_request(instance, "CommitTX", request, handle_contract_call_response);

loc_exit:
  if (call_info) free(call_info);
  if (buffer   ) free(buffer   );
  return status;
}

EXPORTED bool aergo_call_smart_contract_json(aergo *instance, transaction_receipt *receipt, aergo_account *account, const char *contract_address, const char *function, const char *args){
  if (!receipt) return false;
  return aergo_call_smart_contract__int(instance, NULL, NULL, receipt, account, contract_address, function, args);
}

EXPORTED bool aergo_call_smart_contract_json_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *account, const char *contract_address, const char *function, const char *args){
  if (!cb) return false;
  return aergo_call_smart_contract__int(instance, cb, arg, NULL, account, contract_address, function, args);
}

EXPORTED bool aergo_call_smart_contract(aergo *instance, transaction_receipt *receipt, aergo_account *account, const char *contract_address, const char *function, const char *types, ...){
  uint8_t args[512];
  va_list ap;
  bool ret;

  va_start(ap, types);
  ret = arguments_to_json(args, sizeof(args), types, ap);
  va_end(ap);
  if (ret == false) return false;

  return aergo_call_smart_contract_json(instance, receipt, account, contract_address, function, args);
}

EXPORTED bool aergo_call_smart_contract_async(aergo *instance, transaction_receipt_cb cb, void *arg, aergo_account *account, const char *contract_address, const char *function, const char *types, ...){
  uint8_t args[512];
  va_list ap;
  bool ret;

  va_start(ap, types);
  ret = arguments_to_json(args, sizeof(args), types, ap);
  va_end(ap);
  if (ret == false) return false;

  return aergo_call_smart_contract_json_async(instance, cb, arg, account, contract_address, function, args);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static bool aergo_query_smart_contract__int(aergo *instance, query_smart_contract_cb cb, void *arg, char *result, int resultlen, const char *contract_address, const char *function, const char *args){
  uint8_t *query_info=NULL, *buffer=NULL, txn_hash[32];
  size_t size;
  struct request *request = NULL;
  bool status = false;

  DEBUG_PRINTLN("aergo_query_smart_contract");

  if (!instance || !contract_address || !function) return false;

  if (result) result[0] = 0;

  size = strlen(function) + strlen2(args);
  query_info = malloc(size + 32);
  buffer = malloc(size + 300);
  if (!query_info || !buffer) goto loc_exit;

  if (args && strlen(args) > 0) {
    snprintf(query_info, size + 32,
            "{\"Name\":\"%s\",\"Args\":%s}", function, args);
  } else {
    snprintf(query_info, size + 32,
            "{\"Name\":\"%s\"}", function);
  }

  size += 300;
  if (EncodeQuery(buffer, &size, contract_address, query_info) == false) goto loc_exit;

  request = new_request(instance);
  if (!request) goto loc_exit;

  request->data = buffer;
  request->size = size;
  request->callback = cb;
  request->arg = arg;
  request->return_ptr = result;
  request->return_size = resultlen;

  request->process_error = handle_query_error;

  status = send_grpc_request(instance, "QueryContract", request, handle_query_response);

loc_exit:
  if (query_info) free(query_info);
  if (buffer    ) free(buffer    );
  return status;
}

EXPORTED bool aergo_query_smart_contract_json(aergo *instance, char *result, int resultlen, const char *contract_address, const char *function, const char *args){
  if (!result || resultlen <= 0) return false;
  return aergo_query_smart_contract__int(instance, NULL, NULL, result, resultlen, contract_address, function, args);
}

EXPORTED bool aergo_query_smart_contract_json_async(aergo *instance, query_smart_contract_cb cb, void *arg, const char *contract_address, const char *function, const char *args){
  if (!cb) return false;
  return aergo_query_smart_contract__int(instance, cb, arg, NULL, 0, contract_address, function, args);
}

EXPORTED bool aergo_query_smart_contract(aergo *instance, char *result, int resultlen, const char *contract_address, const char *function, const char *types, ...){
  uint8_t args[512], *pargs;
  va_list ap;
  bool ret;

  if (types && types[0]) {
    va_start(ap, types);
    ret = arguments_to_json(args, sizeof(args), types, ap);
    va_end(ap);
    if (ret == false) return false;
    pargs = args;
  } else {
    pargs = NULL;
  }

  return aergo_query_smart_contract_json(instance, result, resultlen, contract_address, function, pargs);
}

EXPORTED bool aergo_query_smart_contract_async(aergo *instance, query_smart_contract_cb cb, void *arg, const char *contract_address, const char *function, const char *types, ...){
  uint8_t args[512], *pargs;
  va_list ap;
  bool ret;

  if (types && types[0]) {
    va_start(ap, types);
    ret = arguments_to_json(args, sizeof(args), types, ap);
    va_end(ap);
    if (ret == false) return false;
    pargs = args;
  } else {
    pargs = NULL;
  }

  return aergo_query_smart_contract_json_async(instance, cb, args, contract_address, function, pargs);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

EXPORTED bool aergo_contract_events_subscribe(aergo *instance, const char *contract_address, const char *event_name, contract_event_cb cb, void *arg){
  uint8_t buffer[256];
  size_t size;
  struct request *request = NULL;

  DEBUG_PRINTLN("aergo_contract_events_subscribe");

  if (!instance || !contract_address || !event_name || !cb) return false;

  size = sizeof(buffer);
  if (EncodeFilterInfo(buffer, &size, contract_address, event_name) == false){
    return false;
  }

  request = new_request(instance);
  if (!request) return false;

  request->data = buffer;
  request->size = size;
  request->callback = cb;
  request->arg = arg;
  request->keep_active = true;

  return send_grpc_request(instance, "ListEventStream", request, handle_event_response);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static bool on_failed_receipt(aergo *instance, struct request *request) {

  DEBUG_PRINTLN("on_failed_receipt");

  if (strstr(request->error_msg, "tx not found")) {
    /* request a new transaction receipt */
    return aergo_get_receipt__int(instance,
           request->txn_hash,
           request->callback,
           request->arg,
           (transaction_receipt *) request->return_ptr,
           true);
  }

  return false;
}

static bool aergo_get_receipt__int(aergo *instance, const char *txn_hash, transaction_receipt_cb cb, void *arg, struct transaction_receipt *receipt, bool retry_on_failure){
  uint8_t buffer[256];
  size_t size;
  struct request *request = NULL;

  DEBUG_PRINTLN("aergo_get_receipt");

  if (!instance || !txn_hash) return false;

  size = sizeof(buffer);
  if (EncodeTxnHash(buffer, &size, txn_hash) == false) return false;

  request = new_request(instance);
  if (!request) return false;

  request->data = buffer;
  request->size = size;
  memcpy(request->txn_hash, txn_hash, 32);
  request->callback = cb;
  request->arg = arg;
  request->return_ptr = receipt;

  if (retry_on_failure) {
    request->process_error = on_failed_receipt;
  }

  return send_grpc_request(instance, "GetReceipt", request, handle_receipt_response);
}

EXPORTED bool aergo_get_receipt(aergo *instance, const char *txn_hash, struct transaction_receipt *receipt){
  if (!receipt) return false;
  return aergo_get_receipt__int(instance, txn_hash, NULL, NULL, receipt, false);
}

EXPORTED bool aergo_get_receipt_async(aergo *instance, const char *txn_hash, transaction_receipt_cb cb, void *arg){
  if (!cb) return false;
  return aergo_get_receipt__int(instance, txn_hash, cb, arg, NULL, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

/*

static bool aergo_get_block__int(aergo *instance, uint64_t blockNo){
  uint8_t buffer[128];
  size_t size;
  struct request *request = NULL;

  DEBUG_PRINTLN("aergo_get_block");

  if (!instance || blockNo==0) return false;

  size = sizeof(buffer);
  if (EncodeBlockNo(buffer, &size, blockNo) == false) return false;

  request = new_request(instance);
  if (!request) return false;

  request->data = buffer;
  request->size = size;
  request->callback = cb;

  return send_grpc_request(instance, "GetBlockMetadata", request, handle_block_response);
}

EXPORTED bool aergo_get_block(aergo *instance, uint64_t blockNo){


}

EXPORTED bool aergo_get_block_async(aergo *instance, uint64_t blockNo){


}

*/

///////////////////////////////////////////////////////////////////////////////////////////////////

/*
EXPORTED bool aergo_block_stream_subscribe(aergo *instance){
  uint8_t buffer[32];
  size_t size;
  struct request *request = NULL;

  DEBUG_PRINTLN("aergo_block_stream_subscribe");

  if (!instance || !cb) return false;

  size = sizeof(buffer);
  if (EncodeEmptyMessage(buffer, &size) == false) return false;

  request = new_request(instance);
  if (!request) return false;

  request->data = buffer;
  request->size = size;
  request->callback = cb;
  request->keep_active = true;

  return send_grpc_request(instance, "ListBlockStream", request, handle_block_response);
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////

EXPORTED bool aergo_get_blockchain_status(aergo *instance){
  uint8_t buffer[128];
  size_t size;
  struct request *request = NULL;

  DEBUG_PRINTLN("aergo_get_blockchain_status");

  if (!instance) return false;

  size = sizeof(buffer);
  if (EncodeEmptyMessage(buffer, &size) == false) return false;

  request = new_request(instance);
  if (!request) return false;

  request->data = buffer;
  request->size = size;

  return send_grpc_request(instance, "Blockchain", request, handle_blockchain_status_response);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

EXPORTED bool aergo_check_privkey(aergo *instance, aergo_account *account){
  if (!instance || !account) return false;
  return secp256k1_ec_seckey_verify(instance->ecdsa, account->privkey);
}

EXPORTED bool aergo_get_account_state(aergo *instance, aergo_account *account, char *error){
  uint8_t buffer[128];
  size_t size;
  struct request *request = NULL;

  DEBUG_PRINTLN("aergo_get_account_state");

  if (!instance || !account) return false;

  if (account->pubkey[0] == 0 && account->pubkey[1] == 0) {
    if (account->use_ledger) {
      if (ledger_get_account_public_key(account, error) == false) {
        return false;
      }
    } else {
      /* calculate the public key */
      secp256k1_pubkey pubkey;
      int ret = secp256k1_ec_pubkey_create(instance->ecdsa, &pubkey, account->privkey);
      if (ret) {
        size_t pklen = 33;
        ret = secp256k1_ec_pubkey_serialize(instance->ecdsa, account->pubkey,
                                          &pklen, &pubkey, SECP256K1_EC_COMPRESSED);
      }
      if (ret != 1) {
        if (error)
          strcpy(error, "failed to get public key from the secret key");
        return false;
      }
    }
  }

  account->is_updated = false;

  size = sizeof(buffer);
  if (EncodeAccountAddress(buffer, &size, account) == false) return false;

  request = new_request(instance);
  if (!request) return false;

  request->data = buffer;
  request->size = size;
  request->account = account;

  send_grpc_request(instance, "GetState", request, handle_account_state_response);

  /* get the account address */
  encode_address(account->pubkey, AddressLength, account->address, sizeof account->address);

  return account->is_updated;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

EXPORTED char * aergo_lib_version() {
  return "0.1.0";
}

EXPORTED aergo * aergo_connect(const char *host, int port) {
  aergo *instance;

#ifdef _WIN32
  WSADATA wsa_data;
  WSAStartup(MAKEWORD(2,2), &wsa_data);
#endif

  DEBUG_PRINTF("aergo_connect %s:%d\n", host, port);

  instance = malloc(sizeof(struct aergo));
  if (!instance) return NULL;

  memset(instance, 0, sizeof(struct aergo));

  strcpy(instance->host, host);
  instance->port = port;
  instance->timeout = 5000; /* default timeout */
  instance->ecdsa = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

  return instance;
}

EXPORTED void aergo_free(aergo *instance) {
  if (instance) {
    if (instance->ecdsa) secp256k1_context_destroy(instance->ecdsa);
    while (instance->requests) free_request(instance, instance->requests);
    free(instance);
  }
}
