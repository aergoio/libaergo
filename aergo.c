#include "aergo.h"

#include "pb_common.h"
#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"

#include "blockchain.pb.h"

#include "endianess.h"
#include "conversions.h"
#include "account.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma GCC diagnostic warning "-fpermissive"

//#define DEBUG_MESSAGES 1

#if defined(DEBUG_MESSAGES)
#define DEBUG_PRINTF     printf
#define DEBUG_PRINTLN    println
#define DEBUG_PRINT_BUFFER print_buffer
#else
#define DEBUG_PRINTF(...)
#define DEBUG_PRINTLN(...)
#define DEBUG_PRINT_BUFFER(...)
#endif

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_exit       exit
#define mbedtls_printf     DEBUG_PRINTF
//#define mbedtls_snprintf   snprintf
#define mbedtls_free       free
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif

#include "mbedtls/ecdsa.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/error.h"

static int ecdsa_rand(void *rng_state, unsigned char *output, size_t len){

#if 0
  while( len > 0 ){
    int rnd;
    size_t use_len = len;
    if( use_len > sizeof(int) )
      use_len = sizeof(int);

    rnd = rand();
    memcpy( output, &rnd, use_len );
    output += use_len;
    len -= use_len;
  }
#endif

  esp_fill_random(output, len);
  return 0;
}

static void print_buffer(const char *title, unsigned char *data, size_t len){
  size_t i;
  DEBUG_PRINTF("%s (%d bytes) ", title, len);
  for(i=0; i<len; i++){
    DEBUG_PRINTF(" %02x", data[i]);
    if(i % 16 == 15) DEBUG_PRINTLN("");
  }
  DEBUG_PRINTLN("");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE KEY STORAGE
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <EEPROM.h>

#define EACH         64
#define EEPROM_SIZE  (4 * EACH) + 2

int aergo_esp32_load_account(aergo_account *account){
  const mbedtls_ecp_curve_info *curve_info = mbedtls_ecp_curve_info_from_grp_id(MBEDTLS_ECP_DP_SECP256K1);
  mbedtls_ecdsa_context *keypair;
  unsigned char buf[EEPROM_SIZE];
  int rc, base, i;

  if( !account ) return -2;

  keypair = &account->keypair;
  mbedtls_ecdsa_init(keypair);

  /* read data from EPROM */
  if (!EEPROM.begin(EEPROM_SIZE)){
    DEBUG_PRINTLN("failed to initialise EEPROM");
    return -1;
  }

  if( EEPROM.read(0)!=53 || EEPROM.read(1)!=79 ){
    DEBUG_PRINTLN("invalid values at EEPROM. generating a new private key");
    goto loc_notset;
  }
  base = 2;

  for (i=0; i<EEPROM_SIZE-base; i++) {
    buf[i] = EEPROM.read(base+i);
  }

  /* check if  */
  mbedtls_ecp_group_load(&keypair->grp, MBEDTLS_ECP_DP_SECP256K1);

  rc = mbedtls_mpi_read_binary(&keypair->d, buf, EACH);
  if (rc) {
    DEBUG_PRINTLN("failed read private key. probably invalid. generating a new one");
    goto loc_notset;
  }
  rc = mbedtls_mpi_read_binary(&keypair->Q.X, &buf[EACH], EACH);
  if (rc) {
    DEBUG_PRINTLN("failed read public key X. probably invalid. generating a new one");
    goto loc_notset;
  }
  rc = mbedtls_mpi_read_binary(&keypair->Q.Y, &buf[EACH*2], EACH);
  if (rc) {
    DEBUG_PRINTLN("failed read public key Y. probably invalid. generating a new one");
    goto loc_notset;
  }
  rc = mbedtls_mpi_read_binary(&keypair->Q.Z, &buf[EACH*3], EACH);
  if (rc) {
    DEBUG_PRINTLN("failed read public key Z. probably invalid. generating a new one");
    goto loc_notset;
  }


  if (rc) {
    DEBUG_PRINTLN("failed read private key. probably invalid. generating a new one");
    loc_notset:

    /* generate a new private key */
    rc = mbedtls_ecdsa_genkey(keypair, curve_info->grp_id, ecdsa_rand, NULL);
    if (rc) return rc;

    /* store the private key on the EPROM */
    rc = mbedtls_mpi_write_binary(&keypair->d, buf, EACH);
    if (rc) return rc;
    rc = mbedtls_mpi_write_binary(&keypair->Q.X, &buf[EACH], EACH);
    if (rc) return rc;
    rc = mbedtls_mpi_write_binary(&keypair->Q.Y, &buf[EACH*2], EACH);
    if (rc) return rc;
    rc = mbedtls_mpi_write_binary(&keypair->Q.Z, &buf[EACH*3], EACH);
    if (rc) return rc;

    DEBUG_PRINTF("writting the private key to EEPROM.");
    EEPROM.write(0, 53);
    EEPROM.write(1, 79);
    for (i=0; i<EEPROM_SIZE-2; i++) {
      EEPROM.write(i+2, buf[i]);
      DEBUG_PRINTF(".");
    }
    EEPROM.commit();
    DEBUG_PRINTLN(" done");

  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// DECODING
///////////////////////////////////////////////////////////////////////////////////////////////////

bool request_finished = false;

uint8_t null_byte[1] = {0};

unsigned char* to_send = NULL;
int send_size = 0;

struct blob {
  uint8_t *ptr;
  size_t  size;
};

bool print_string(pb_istream_t *stream, const pb_field_t *field, void **arg){
    uint8_t buffer[1024] = {0};

    DEBUG_PRINTF("print_string stream->bytes_left=%d field=%p\n", stream->bytes_left, field);

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

    DEBUG_PRINTF("read_string arg=%p\n", str);
    if (!str) return true;
    DEBUG_PRINTF("read_string bytes_left=%d str->size=%d\n", stream->bytes_left, str->size);

    /* We could read block-by-block to avoid the large buffer... */
    if (stream->bytes_left > str->size || stream->bytes_left < 0){
        DEBUG_PRINTF("FAILED! read_string\n");
        return false;
    }

    if (!pb_read(stream, str->ptr, stream->bytes_left))
        return false;

    str->ptr[len] = 0;  // null terminator

    DEBUG_PRINTF("read_string ok\n");
    return true;
}

bool read_blob(pb_istream_t *stream, const pb_field_t *field, void **arg){
    struct blob *blob = *(struct blob**)arg;

    DEBUG_PRINTF("read_blob arg=%p\n", blob);
    if (!blob) return true;
    DEBUG_PRINTF("read_blob bytes_left=%d blob->size=%d\n", stream->bytes_left, blob->size);

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

    //DEBUG_PRINTF("print_blob stream->bytes_left=%d field=%p\n", stream->bytes_left, field);

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
    DEBUG_PRINTF("read_bignum_to_double bytes_left=%d\n", stream->bytes_left);

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
    *pvalue = atof(buf);

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

    DEBUG_PRINTF("encode_fixed64 - value=%llu\n", value);

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

    DEBUG_PRINTF("encode_varuint64 - value=%llu\n", value);

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    // convert the value to big endian
    copy_be64(&value2, &value);

    // skip the null bytes, unless the last one
    ptr = (uint8_t*)&value2;
    len = 8;
    while( *ptr==0 && len>1 ){ ptr++; len--; }

    DEBUG_PRINTF("encode_varuint64 - len=%u\n", len);

    return pb_encode_string(stream, ptr, len);
}

bool encode_account_address(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
    char *str = *(char**)arg;
    char decoded[128];
    bool res;

    DEBUG_PRINTF("encode_account_address '%s'\n", str);

    if (strlen(str) != EncodedAddressLength) {
      DEBUG_PRINTF("Lenght of address is invalid: %d. It should be %d\n", strlen(str), EncodedAddressLength);
    }

    res = decode_address(str, strlen(str), decoded, sizeof(decoded));

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, (uint8_t*)decoded, AddressLength);
}

bool copy_ecdsa_address(mbedtls_ecdsa_context *account, uint8_t *buf, size_t bufsize) {
    size_t len;
    bool ret;

    ret = mbedtls_ecp_point_write_binary(&account->grp, &account->Q,
                MBEDTLS_ECP_PF_COMPRESSED, &len, buf, bufsize);

    DEBUG_PRINTF("copy_ecdsa_address - ret=%d len=%d\n", ret, len);

    return ret && (len == AddressLength);
}

bool encode_ecdsa_address(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
    mbedtls_ecdsa_context *account = *(mbedtls_ecdsa_context**)arg;
    uint8_t buf[128];
    size_t len;
    bool ret;

    ret = mbedtls_ecp_point_write_binary(&account->grp, &account->Q,
                MBEDTLS_ECP_PF_COMPRESSED, &len, buf, sizeof buf);

    if( ret != 0 ){
        DEBUG_PRINTF("encode_ecdsa_address - invalid account\n");
        return false;
    }

    DEBUG_PRINTF("encode_ecdsa_address - len=%d\n", len);

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, buf, len);
}

bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
    char *str = *(char**)arg;

    DEBUG_PRINTF("encode_string '%s'\n", str);

    if (!arg) return true;

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, (uint8_t*)str, strlen(str));
}

bool encode_blob(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
    struct blob *blob = *(struct blob**)arg;

    DEBUG_PRINTF("encode_blob arg=%p\n", blob);

    if (!blob) return true;

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    if (blob->ptr==0) return true;
    return pb_encode_string(stream, blob->ptr, blob->size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// HTTP2 REQUEST CALLBACKS
///////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t blockchain_id_hash[32] = {0};

aergo_account *arg_aergo_account;

struct transaction_receipt *arg_receipt;

struct blob arg_str;

bool arg_success;

int handle_blockchain_status_response(struct sh2lib_handle *handle, const char *data, size_t len, int flags) {
    if (len > 0) {
        int i, ret;
        BlockchainStatus status = BlockchainStatus_init_zero;

        DEBUG_PRINT_BUFFER("returned", data, len);

        /* Create a stream that reads from the buffer */
        pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

        /* Set the callback functions */
        struct blob bb { .ptr = blockchain_id_hash, .size = 32 };
        status.best_chain_id_hash.arg = &bb;
        status.best_chain_id_hash.funcs.decode = &read_blob;

        /* Now we are ready to decode the message */
        ret = pb_decode(&stream, BlockchainStatus_fields, &status);

        /* Check for errors... */
        if (!ret) {
            DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }

        /* Print the data contained in the message */
        //DEBUG_PRINTF("Block number: %llu\n", block.header.blockNo);
        DEBUG_PRINT_BUFFER("  + ChainIdHash: ", blockchain_id_hash, sizeof(blockchain_id_hash));

    } else {
        DEBUG_PRINTLN("returned 0 bytes");
    }

    if (flags == DATA_RECV_FRAME_COMPLETE) {
        request_finished = true;
        DEBUG_PRINTLN("COMPLETE FRAME RECEIVED");
    } else if (flags == DATA_RECV_RST_STREAM) {
        request_finished = true;
        DEBUG_PRINTLN("STREAM CLOSED");
    }
    return 0;
}

int handle_account_state_response(struct sh2lib_handle *handle, const char *data, size_t len, int flags) {
    if (len > 0) {
        int i, ret;
        State account_state = State_init_zero;

        DEBUG_PRINT_BUFFER("returned", data, len);

        /* Create a stream that reads from the buffer */
        pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

        /* Set the callback functions */

        account_state.balance.arg = &arg_aergo_account->balance;
        account_state.balance.funcs.decode = &read_bignum_to_double;

        struct blob bb { .ptr = arg_aergo_account->state_root, .size = 32 };
        account_state.storageRoot.arg = &bb;
        account_state.storageRoot.funcs.decode = &read_blob;

        /* Now we are ready to decode the message */
        ret = pb_decode(&stream, State_fields, &account_state);

        /* Check for errors... */
        if (!ret) {
            DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }

        arg_aergo_account->is_updated = true;
        arg_aergo_account->nonce = account_state.nonce;

        /* Print the data contained in the message */
        DEBUG_PRINTF("Account Nonce: %llu\n", account_state.nonce);
        //DEBUG_PRINT_BUFFER("  + ChainIdHash: ", storageRoot, sizeof(storageRoot));

    } else {
        DEBUG_PRINTLN("returned 0 bytes");
    }

    if (flags == DATA_RECV_FRAME_COMPLETE) {
        request_finished = true;
        DEBUG_PRINTLN("COMPLETE FRAME RECEIVED");
    } else if (flags == DATA_RECV_RST_STREAM) {
        request_finished = true;
        DEBUG_PRINTLN("STREAM CLOSED");
    }
    return 0;
}

int handle_block_response(struct sh2lib_handle *handle, const char *data, size_t len, int flags) {
    if (len > 0) {
        int i, status;
        Block block = Block_init_zero;

        DEBUG_PRINT_BUFFER("returned", data, len);

        /* Create a stream that reads from the buffer */
        pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

        /* Set the callback functions */
        block.header.chainID.funcs.decode = &print_blob;
        block.header.chainID.arg = (void*)"chainID";
        block.header.pubKey.funcs.decode = &print_blob;
        block.header.pubKey.arg = (void*)"pubKey";
        block.body.txs.funcs.decode = &print_string;
        block.body.txs.arg = (void*)"txs";

        /* Now we are ready to decode the message */
        status = pb_decode(&stream, Block_fields, &block);

        /* Check for errors... */
        if (!status) {
            DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }

        /* Print the data contained in the message */
        DEBUG_PRINTF("Block number: %llu\n", block.header.blockNo);
        DEBUG_PRINTF("Block timestamp: %llu\n", block.header.timestamp);
        DEBUG_PRINTF("Block confirms: %llu\n", block.header.confirms);

    } else {
        DEBUG_PRINTLN("returned 0 bytes");
    }

    if (flags == DATA_RECV_FRAME_COMPLETE) {
        DEBUG_PRINTLN("COMPLETE FRAME RECEIVED");
    } else if (flags == DATA_RECV_RST_STREAM) {
        request_finished = true;
        DEBUG_PRINTLN("STREAM CLOSED");
    }
    return 0;
}

int handle_transfer_response(struct sh2lib_handle *handle, const char *data, size_t len, int flags) {
    if (len > 0) {
        int i, status;
        CommitResultList response = CommitResultList_init_zero;

        DEBUG_PRINT_BUFFER("returned", data, len);

        /* Create a stream that reads from the buffer */
        pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

        /* Set the callback functions */
        //response.results.funcs.decode = &decode_commit_result;
        //response.results.arg = ...;

        /* Now we are ready to decode the message */
        status = pb_decode(&stream, CommitResultList_fields, &response);

        /* Check for errors... */
        if (!status) {
            DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }

        /* Print the data contained in the message */
        DEBUG_PRINTF("response error status: %u\n", response.results.error);

        arg_success = (response.results.error == CommitStatus_TX_OK);

    } else {
        DEBUG_PRINTLN("returned 0 bytes");
    }

    if (flags == DATA_RECV_FRAME_COMPLETE) {
        request_finished = true;
        DEBUG_PRINTLN("COMPLETE FRAME RECEIVED");
    } else if (flags == DATA_RECV_RST_STREAM) {
        request_finished = true;
        DEBUG_PRINTLN("STREAM CLOSED");
    }
    return 0;
}

int handle_contract_call_response(struct sh2lib_handle *handle, const char *data, size_t len, int flags) {
    if (len > 0) {
        int i, status;
        CommitResultList response = CommitResultList_init_zero;

        DEBUG_PRINT_BUFFER("returned", data, len);

        /* Create a stream that reads from the buffer */
        pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

        /* Set the callback functions */
        //response.results.funcs.decode = &decode_commit_result;
        //response.results.arg = ...;

        /* Now we are ready to decode the message */
        status = pb_decode(&stream, CommitResultList_fields, &response);

        /* Check for errors... */
        if (!status) {
            DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }

        /* Print the data contained in the message */
        DEBUG_PRINTF("response error status: %u\n", response.results.error);

        arg_success = (response.results.error == CommitStatus_TX_OK);

    } else {
        DEBUG_PRINTLN("returned 0 bytes");
    }

    if (flags == DATA_RECV_FRAME_COMPLETE) {
        request_finished = true;
        DEBUG_PRINTLN("COMPLETE FRAME RECEIVED");
    } else if (flags == DATA_RECV_RST_STREAM) {
        request_finished = true;
        DEBUG_PRINTLN("STREAM CLOSED");
    }
    return 0;
}

int handle_query_response(struct sh2lib_handle *handle, const char *data, size_t len, int flags) {
    if (len > 0) {
        int i, status;
        SingleBytes response = SingleBytes_init_zero;

        DEBUG_PRINT_BUFFER("returned", data, len);

        /* Create a stream that reads from the buffer */
        pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

        /* Set the callback functions */
        response.value.arg = &arg_str;
        response.value.funcs.decode = &read_string;

        /* Now we are ready to decode the message */
        status = pb_decode(&stream, SingleBytes_fields, &response);

        /* Check for errors... */
        if (!status) {
            DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }

        arg_success = true;

    } else {
        DEBUG_PRINTLN("returned 0 bytes");
    }

    if (flags == DATA_RECV_FRAME_COMPLETE) {
        request_finished = true;
        DEBUG_PRINTLN("COMPLETE FRAME RECEIVED");
    } else if (flags == DATA_RECV_RST_STREAM) {
        request_finished = true;
        DEBUG_PRINTLN("STREAM CLOSED");
    }
    return 0;
}

contract_event_cb arg_contract_event_cb = NULL;

int handle_event_response(struct sh2lib_handle *handle, const char *data, size_t len, int flags) {
    if (len > 0) {
        Event response = Event_init_zero;
        contract_event event = {0};
        char raw_address[64];
        int status;

        DEBUG_PRINT_BUFFER("returned", data, len);

        if (arg_contract_event_cb==NULL) {
          DEBUG_PRINTLN("STREAM CLOSED");
          request_finished = true;
          return 0;
        }

        /* Create a stream that reads from the buffer */
        pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

        /* Set the callback functions */

        struct blob s1 { .ptr = (uint8_t*) raw_address, .size = sizeof raw_address };
        response.contractAddress.arg = &s1;
        response.contractAddress.funcs.decode = &read_blob;

        struct blob s2 { .ptr = (uint8_t*) event.eventName, .size = sizeof event.eventName };
        response.eventName.arg = &s2;
        response.eventName.funcs.decode = &read_string;

        struct blob s3 { .ptr = (uint8_t*) event.jsonArgs, .size = sizeof event.jsonArgs };
        response.jsonArgs.arg = &s3;
        response.jsonArgs.funcs.decode = &read_string;

        struct blob b1 { .ptr = (uint8_t*) event.txHash, .size = sizeof event.txHash };
        response.txHash.arg = &b1;
        response.txHash.funcs.decode = &read_blob;

        struct blob b2 { .ptr = (uint8_t*) event.blockHash, .size = sizeof event.blockHash };
        response.blockHash.arg = &b2;
        response.blockHash.funcs.decode = &read_blob;

        /* Now we are ready to decode the message */
        status = pb_decode(&stream, Event_fields, &response);
        if (!status) {
            DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }

        /* encode the contract address from binary to string format */
        encode_address(raw_address, AddressLength, event.contractAddress, sizeof event.contractAddress);

        /* copy values */
        event.eventIdx = response.eventIdx;
        event.blockNo = response.blockNo;
        event.txIndex = response.txIndex;

        /* Call the callback function */
        arg_contract_event_cb(&event);

        arg_success = true;

    } else {
        DEBUG_PRINTLN("returned 0 bytes");
    }

    if (flags == DATA_RECV_FRAME_COMPLETE) {
        DEBUG_PRINTLN("COMPLETE FRAME RECEIVED");
    } else if (flags == DATA_RECV_RST_STREAM) {
        request_finished = true;
        DEBUG_PRINTLN("STREAM CLOSED");
    }
    return 0;
}

int handle_receipt_response(struct sh2lib_handle *handle, const char *data, size_t len, int flags) {
    if (len > 0) {
        Receipt response = Receipt_init_zero;
        transaction_receipt *receipt = arg_receipt;
        char raw_address[64];
        int status;

        memset(receipt, 0, sizeof(struct transaction_receipt));

        DEBUG_PRINT_BUFFER("returned", data, len);

        /* Create a stream that reads from the buffer */
        pb_istream_t stream = pb_istream_from_buffer((const unsigned char *)&data[5], len-5);

        /* Set the callback functions */

        struct blob s1 { .ptr = (uint8_t*) raw_address, .size = sizeof raw_address };
        response.contractAddress.arg = &s1;
        response.contractAddress.funcs.decode = &read_blob;

        struct blob s2 { .ptr = (uint8_t*) receipt->status, .size = sizeof receipt->status };
        response.status.arg = &s2;
        response.status.funcs.decode = &read_string;

        struct blob s3 { .ptr = (uint8_t*) receipt->ret, .size = sizeof receipt->ret };
        response.ret.arg = &s3;
        response.ret.funcs.decode = &read_string;

        struct blob b1 { .ptr = (uint8_t*) receipt->txHash, .size = sizeof receipt->txHash };
        response.txHash.arg = &b1;
        response.txHash.funcs.decode = &read_blob;

        struct blob b2 { .ptr = (uint8_t*) receipt->blockHash, .size = sizeof receipt->blockHash };
        response.blockHash.arg = &b2;
        response.blockHash.funcs.decode = &read_blob;

        response.feeUsed.arg = &receipt->feeUsed;
        response.feeUsed.funcs.decode = &read_bignum_to_double;

        /* Now we are ready to decode the message */
        status = pb_decode(&stream, Receipt_fields, &response);
        if (!status) {
            DEBUG_PRINTF("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }

        /* encode the contract address from binary to string format */
        encode_address(raw_address, AddressLength, receipt->contractAddress, sizeof receipt->contractAddress);

        /* copy values */
        receipt->gasUsed = response.gasUsed;
        receipt->blockNo = response.blockNo;
        receipt->txIndex = response.txIndex;
        receipt->feeDelegation = response.feeDelegation;

        arg_success = true;

    } else {
        DEBUG_PRINTLN("returned 0 bytes");
    }

    if (flags == DATA_RECV_FRAME_COMPLETE) {
        DEBUG_PRINTLN("COMPLETE FRAME RECEIVED");
    } else if (flags == DATA_RECV_RST_STREAM) {
        request_finished = true;
        DEBUG_PRINTLN("STREAM CLOSED");
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t encode_http2_data_frame(uint8_t *buffer, uint32_t content_size);

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
  unsigned char sign[MBEDTLS_ECDSA_MAX_LEN]; // sender's signature for this TxBody
  size_t sig_len;
  unsigned char hash[32];       // hash of the whole transaction including the signature
};

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
  uint8_t buf[1024], *ptr;
  size_t len = 0;

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

  return true;
}

bool sign_transaction(struct txn *txn, mbedtls_ecdsa_context *account){
  uint8_t hash[32];
  bool ret;

  calculate_tx_hash(txn, hash, false);

  DEBUG_PRINTLN("sign_transaction");
  DEBUG_PRINT_BUFFER("  hash", hash, sizeof(hash));

  // Sign the message hash
  ret = mbedtls_ecdsa_write_signature(account, MBEDTLS_MD_SHA256,
                                      hash, sizeof(hash),
                                      txn->sign, &txn->sig_len,
                                      ecdsa_rand, NULL);
  if (ret == 0) {
    DEBUG_PRINT_BUFFER("  signature", txn->sign, txn->sig_len);
  } else {
    DEBUG_PRINTF("write_signature FAILED: %d\n", ret);
  }

  return (ret == 0);
}

bool encode_1_transaction(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
  struct txn *txn = *(struct txn **)arg;
  Tx message = Tx_init_zero;

  if (!pb_encode_tag_for_field(stream, field))
      return false;

  calculate_tx_hash(txn, txn->hash, true);

  /* Set the values and the encoder callback functions */
  struct blob bb { .ptr = txn->hash, .size = 32 };
  message.hash.arg = &bb;
  message.hash.funcs.encode = &encode_blob;


  /* Set the values and the encoder callback functions */
  message.body.type = (TxType) txn->type;
  message.body.nonce = txn->nonce;

  struct blob acc { .ptr = txn->account, .size = AddressLength };
  message.body.account.arg = &acc;
  message.body.account.funcs.encode = encode_blob;

  struct blob rec { .ptr = txn->recipient, .size = AddressLength };
  message.body.recipient.arg = &rec;
  message.body.recipient.funcs.encode = encode_blob;

  message.body.payload.arg = txn->payload;
  message.body.payload.funcs.encode = &encode_string;

  struct blob amt { .ptr = txn->amount, .size = txn->amount_len };
  message.body.amount.arg = &amt;
  message.body.amount.funcs.encode = &encode_blob;

  message.body.gasLimit = txn->gasLimit;

  message.body.gasPrice.arg = &txn->gasPrice;
  message.body.gasPrice.funcs.encode = &encode_varuint64;

  struct blob cid { .ptr = txn->chainIdHash, .size = 32 };
  message.body.chainIdHash.arg = &cid;
  message.body.chainIdHash.funcs.encode = &encode_blob;

  struct blob sig { .ptr = txn->sign, .size = txn->sig_len };
  message.body.sign.arg = &sig;
  message.body.sign.funcs.encode = &encode_blob;


  /* Now we are ready to decode the message */
  bool status = pb_encode_submessage(stream, Tx_fields, &message);
  if (!status) {
    DEBUG_PRINTF("Encoding failed: %s\n", PB_GET_ERROR(stream));
  }
  return status;
}

bool encode_transaction(uint8_t *buffer, size_t *psize, char *txn_hash, struct txn *txn) {
  TxList message = TxList_init_zero;
  uint32_t size;

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Set the values and the encoder callback functions */
  message.txs.arg = txn;
  message.txs.funcs.encode = &encode_1_transaction;

  /* Now we are ready to decode the message */
  bool status = pb_encode(&stream, TxList_fields, &message);
  if (!status) {
    DEBUG_PRINTF("Encoding failed: %s\n", PB_GET_ERROR(&stream));
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
// COMMAND ENCODING
///////////////////////////////////////////////////////////////////////////////////////////////////

bool EncodeTransfer(uint8_t *buffer, size_t *psize, char *txn_hash, aergo_account *account, char *recipient, uint8_t *amount, int amount_len) {
  struct txn txn;
  char out[64]={0};

  if (!amount || amount_len < 1) return false;

  /* increment the account nonce */
  account->nonce++;

  txn.nonce = account->nonce;
  copy_ecdsa_address(&account->keypair, txn.account, sizeof txn.account);
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
  DEBUG_PRINTF("account nonce: %llu\n", account->nonce);

  if (sign_transaction(&txn, &account->keypair) == false) {
    return false;
  }

  return encode_transaction(buffer, psize, txn_hash, &txn);
}

bool EncodeContractCall(uint8_t *buffer, size_t *psize, char *txn_hash, char *contract_address, char *call_info, aergo_account *account) {
  struct txn txn;
  char out[64]={0};

  /* increment the account nonce */
  account->nonce++;

  txn.nonce = account->nonce;
  copy_ecdsa_address(&account->keypair, txn.account, sizeof txn.account);
  decode_address(contract_address, strlen(contract_address), txn.recipient, sizeof(txn.recipient));
  txn.amount = null_byte;   // variable-length big-endian integer
  txn.amount_len = 1;
  txn.payload = call_info;
  txn.gasLimit = 0;
  txn.gasPrice = 0;          // variable-length big-endian integer
  txn.type = TxType_CALL;
  txn.chainIdHash = blockchain_id_hash;

  encode_address(txn.account, sizeof txn.account, out, sizeof out);
  DEBUG_PRINTF("account address: %s\n", out);
  DEBUG_PRINTF("account nonce: %llu\n", account->nonce);

  if (sign_transaction(&txn, &account->keypair) == false) {
    return false;
  }

  return encode_transaction(buffer, psize, txn_hash, &txn);
}

bool EncodeQuery(uint8_t *buffer, size_t *psize, char *contract_address, char *query_info){
  Query message = Query_init_zero;
  uint32_t size;

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Set the callback functions */
  message.contractAddress.funcs.encode = &encode_account_address;
  message.contractAddress.arg = contract_address;
  message.queryinfo.funcs.encode = &encode_string;
  message.queryinfo.arg = query_info;

  /* Now we are ready to decode the message */
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

bool EncodeFilterInfo(uint8_t *buffer, size_t *psize, char *contract_address, char *event_name){
  FilterInfo message = FilterInfo_init_zero;
  uint32_t size;

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

bool EncodeAccountAddress(uint8_t *buffer, size_t *psize, mbedtls_ecdsa_context *account) {
  SingleBytes message = SingleBytes_init_zero;
  uint32_t size;

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Set the callback functions */
  message.value.funcs.encode = &encode_ecdsa_address;
  message.value.arg = account;

  /* Now we are ready to decode the message */
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

bool EncodeTxnHash(uint8_t *buffer, size_t *psize, char *txn_hash){
  SingleBytes message = SingleBytes_init_zero;
  uint32_t size;

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Set the callback functions */
  struct blob bb { .ptr = txn_hash, .size = 32 };
  message.value.arg = &bb;
  message.value.funcs.encode = &encode_blob;

  /* Now we are ready to decode the message */
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

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Set the callback functions */
  message.value.funcs.encode = &encode_fixed64;
  message.value.arg = &blockNo;

  /* Now we are ready to decode the message */
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

  /* Create a stream that writes to the buffer */
  pb_ostream_t stream = pb_ostream_from_buffer(&buffer[5], *psize - 5);

  /* Now we are ready to decode the message */
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
// HTTP2 AND GRPC
///////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t encode_http2_data_frame(uint8_t *buffer, uint32_t content_size){
  // no compression
  buffer[0] = 0;
  // insert the size in the stream as big endian 32-bit integer
  copy_be32((uint32_t*)&buffer[1], &content_size);
  // return the frame size
  return content_size + 5;
}

int send_post_data(struct sh2lib_handle *handle, char *buf, size_t length, uint32_t *data_flags){
  int copylen = send_size;
  int i;

  DEBUG_PRINTF("send_post_data length=%d\n", length);

  if (copylen <= length) {
    memcpy(buf, to_send, copylen);
  } else {
    copylen = 0;
  }

  DEBUG_PRINT_BUFFER("sending", buf, copylen);

  (*data_flags) |= NGHTTP2_DATA_FLAG_EOF;
  return copylen;
}

void send_grpc_request(struct sh2lib_handle *hd, char *service, uint8_t *buffer, size_t size, sh2lib_frame_data_recv_cb_t response_callback) {
  char path[64];
  char len[8];

  to_send = buffer;
  send_size = size;

  sprintf(path, "/types.AergoRPCService/%s", service);
  sprintf(len, "%d", size);

  const nghttp2_nv nva[] = {
    SH2LIB_MAKE_NV(":method", "POST"),
    SH2LIB_MAKE_NV(":scheme", "https"),
    SH2LIB_MAKE_NV(":authority", hd->hostname),
    SH2LIB_MAKE_NV(":path", path),
    //SH2LIB_MAKE_NV("te", "trailers"),
    SH2LIB_MAKE_NV("Content-Type", "application/grpc"),
    //SH2LIB_MAKE_NV("grpc-encoding", "identity")
    SH2LIB_MAKE_NV("content-length", len)
  };

  request_finished = false;
  sh2lib_do_putpost_with_nv(hd, nva, sizeof(nva) / sizeof(nva[0]), send_post_data, response_callback);

  while (!request_finished) {
    DEBUG_PRINTLN("sh2lib_execute");
    if (sh2lib_execute(hd) != ESP_OK) {
      DEBUG_PRINTLN("Error in execute");
      break;
    }
    vTaskDelay(25);
  }

  DEBUG_PRINTLN("Request done. returning");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool check_blockchain_id_hash(aergo *instance) {

  if (blockchain_id_hash[0] == 0) {
    aergo_get_blockchain_status(instance);
  }

  return (blockchain_id_hash[0] != 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EXPORTED FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////

bool aergo_transfer_bignum(aergo *instance, char *txn_hash, aergo_account *from_account, char *to_account, char *amount, int len){
  uint8_t buffer[1024];
  size_t size;

  if (check_blockchain_id_hash(instance) == false) return false;

  // check if nonce was retrieved
  if ( !from_account->is_updated ){
    if ( aergo_get_account_state(instance, from_account) == false ) return false;
  }

  arg_success = false;
  size = sizeof(buffer);
  if (EncodeTransfer(buffer, &size, txn_hash, from_account, to_account, amount, len)){
    arg_aergo_account = from_account;
    send_grpc_request(&instance->hd, "CommitTX", buffer, size, handle_transfer_response);
  }

  return arg_success;
}

bool aergo_transfer_str(aergo *instance, char *txn_hash, aergo_account *from_account, char *to_account, char *value){
  char buf[16];
  int len;

  len = string_to_bignum(value, strlen(value), buf, sizeof(buf));

  return aergo_transfer_bignum(instance, txn_hash, from_account, to_account, buf, len);

}

bool aergo_transfer(aergo *instance, char *txn_hash, aergo_account *from_account, char *to_account, double value){
  char amount_str[36];

  snprintf(amount_str, sizeof(amount_str), "%f", value);

  return aergo_transfer_str(instance, txn_hash, from_account, to_account, amount_str);

}

bool aergo_transfer_int(aergo *instance, char *txn_hash, aergo_account *from_account, char *to_account, uint64_t integer, uint64_t decimal){
  char amount_str[36];

  snprintf(amount_str, sizeof(amount_str),
           "%lld.%018lld", integer, decimal);

  return aergo_transfer_str(instance, txn_hash, from_account, to_account, amount_str);

}

bool aergo_call_smart_contract_json(aergo *instance, char *txn_hash, aergo_account *account, char *contract_address, char *function, char *args){
  uint8_t call_info[512], buffer[1024];
  size_t size;

  if (check_blockchain_id_hash(instance) == false) return false;

  // check if nonce was retrieved
  if ( !account->is_updated ){
    if ( aergo_get_account_state(instance, account) == false ) return false;
  }

  snprintf(call_info, sizeof(call_info),
           "{\"Name\":\"%s\", \"Args\":%s}", function, args);

  arg_success = false;
  size = sizeof(buffer);
  if (EncodeContractCall(buffer, &size, txn_hash, contract_address, call_info, account)){
    arg_aergo_account = account;
    send_grpc_request(&instance->hd, "CommitTX", buffer, size, handle_contract_call_response);
  }

  return arg_success;
}

bool aergo_call_smart_contract(aergo *instance, char *txn_hash, aergo_account *account, char *contract_address, char *function, char *types, ...){
  uint8_t args[512];
  va_list ap;
  bool ret;

  va_start(ap, types);
  ret = arguments_to_json(args, sizeof(args), types, ap);
  va_end(ap);
  if (ret == false) return false;

  return aergo_call_smart_contract_json(instance, txn_hash, account, contract_address, function, args);
}

bool aergo_query_smart_contract_json(aergo *instance, char *result, int resultlen, char *contract_address, char *function, char *args){
  uint8_t query_info[512], buffer[512];
  size_t size;

  if (args) {
    snprintf(query_info, sizeof(query_info),
            "{\"Name\":\"%s\", \"Args\":%s}", function, args);
  } else {
    snprintf(query_info, sizeof(query_info),
            "{\"Name\":\"%s\"}", function);
  }

  arg_str.ptr = result;
  arg_str.size = resultlen;
  arg_success = false;

  size = sizeof(buffer);
  if (EncodeQuery(buffer, &size, contract_address, query_info)){
    send_grpc_request(&instance->hd, "QueryContract", buffer, size, handle_query_response);
  }

  return arg_success;
}

bool aergo_query_smart_contract(aergo *instance, char *result, int resultlen, char *contract_address, char *function, char *types, ...){
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

bool aergo_contract_events_subscribe(aergo *instance, char *contract_address, char *event_name, contract_event_cb cb){
  uint8_t buffer[256];
  size_t size;

  arg_contract_event_cb = cb;
  arg_success = false;

  size = sizeof(buffer);
  if (EncodeFilterInfo(buffer, &size, contract_address, event_name)){
    send_grpc_request(&instance->hd, "ListEventStream", buffer, size, handle_event_response);
  }

  return arg_success;
}

bool aergo_get_receipt(aergo *instance, char *txn_hash, struct transaction_receipt *receipt){
  uint8_t buffer[256];
  size_t size;

  if (!instance || !txn_hash || !receipt) return false;

  arg_receipt = receipt;
  arg_success = false;

  size = sizeof(buffer);
  if (EncodeTxnHash(buffer, &size, txn_hash)){
    send_grpc_request(&instance->hd, "GetReceipt", buffer, size, handle_receipt_response);
  }

  return arg_success;
}

void aergo_get_block(aergo *instance, uint64_t blockNo){
  uint8_t buffer[128];
  size_t size;

  size = sizeof(buffer);
  if (EncodeBlockNo(buffer, &size, blockNo)){
    send_grpc_request(&instance->hd, "GetBlockMetadata", buffer, size, handle_block_response);
  }

}

void aergo_block_stream_subscribe(aergo *instance){
  uint8_t buffer[128];
  size_t size;

  size = sizeof(buffer);
  if (EncodeEmptyMessage(buffer, &size)){
    send_grpc_request(&instance->hd, "ListBlockStream", buffer, size, handle_block_response);
  }

}

void aergo_get_blockchain_status(aergo *instance){
  uint8_t buffer[128];
  size_t size;

  size = sizeof(buffer);
  if (EncodeEmptyMessage(buffer, &size)){
    send_grpc_request(&instance->hd, "Blockchain", buffer, size, handle_blockchain_status_response);
  }

}

bool aergo_get_account_state(aergo *instance, aergo_account *account){
  uint8_t buffer[128];
  size_t size;

  account->is_updated = false;

  size = sizeof(buffer);
  if (EncodeAccountAddress(buffer, &size, &account->keypair)){
    arg_aergo_account = account;
    send_grpc_request(&instance->hd, "GetState", buffer, size, handle_account_state_response);
  }

  // copy values to the account structure

  copy_ecdsa_address(&account->keypair, buffer, sizeof buffer);
  encode_address(buffer, AddressLength, account->address, sizeof account->address);

  return account->is_updated;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

int aergo_connect(aergo *instance, char *host) {
  return sh2lib_connect(&instance->hd, host);
}

void aergo_free(aergo *instance) {
  sh2lib_free(&instance->hd);
}

void aergo_free_account(aergo_account *account) {
  mbedtls_ecdsa_free(&account->keypair);
}
